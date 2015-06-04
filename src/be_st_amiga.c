#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "SDL.h"
#include <libraries/lowlevel.h>
#include <devices/input.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <exec/interrupts.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include "debug_amiga.h"

#include "be_cross.h"
#include "be_st.h"

#define BE_ST_MAXJOYSTICKS 8
#define BE_ST_EMU_JOYSTICK_RANGEMAX 5000 // id_in.c MaxJoyValue
#define BE_ST_DEFAULT_FARPTRSEGOFFSET 0x14

#if (defined REFKEEN_VER_CATARM) || (defined REFKEEN_VER_CATAPOC)
#define BE_ST_ENABLE_FARPTR_CFG 1
#endif

#if defined(__AMIGA__)
#define IPTR ULONG
#endif

static void (*g_sdlKeyboardInterruptFuncPtr)(uint8_t) = 0;
extern struct Screen *g_amigaScreen;

uint8_t g_sdlLastKeyScanCode;
uint16_t g_mouseButtonState = 0;
int16_t g_mx = 0;
int16_t g_my = 0;

static struct Interrupt g_inputHandler;
static struct MsgPort *g_inputPort = NULL;
static struct IOStdReq *g_inputReq = NULL;
//UWORD *g_pointerMem = NULL;

void BE_ST_InitGfx(void);
void BE_ST_InitAudio(void);
void BE_ST_ShutdownAudio(void);
void BE_ST_ShutdownGfx(void);


typedef struct {
	bool isSpecial; // Scancode of 0xE0 sent?
	uint8_t dosScanCode;
} emulatedDOSKeyEvent;

#define emptyDOSKeyEvent {false, 0}

const emulatedDOSKeyEvent sdlKeyMappings[128] = {
	{false,  BE_ST_SC_GRAVE},
	{false,  BE_ST_SC_1},
	{false,  BE_ST_SC_2},
	{false,  BE_ST_SC_3},
	{false,  BE_ST_SC_4},
	{false,  BE_ST_SC_5},
	{false,  BE_ST_SC_6},
	{false,  BE_ST_SC_7},
	{false,  BE_ST_SC_8},
	{false,  BE_ST_SC_9},
	{false,  BE_ST_SC_0},
	{false,  BE_ST_SC_MINUS},
	{false,  BE_ST_SC_EQUALS},
	{false,  BE_ST_SC_BACKSLASH},
	emptyDOSKeyEvent,
	{false,  BE_ST_SC_KP_0}, //{true,  BE_ST_SC_INSERT},
	{false,  BE_ST_SC_Q},
	{false,  BE_ST_SC_W},
	{false,  BE_ST_SC_E},
	{false,  BE_ST_SC_R},
	{false,  BE_ST_SC_T},
	{false,  BE_ST_SC_Y},
	{false,  BE_ST_SC_U},
	{false,  BE_ST_SC_I},
	{false,  BE_ST_SC_O},
	{false,  BE_ST_SC_P},
	{false,  BE_ST_SC_LBRACKET},
	{false,  BE_ST_SC_RBRACKET},
	emptyDOSKeyEvent,
	{false,  BE_ST_SC_KP_1}, //{true,  BE_ST_SC_END},
	{false,  BE_ST_SC_KP_2},
	{false,  BE_ST_SC_KP_3}, //{true,  BE_ST_SC_PAGEDOWN},
	{false,  BE_ST_SC_A},
	{false,  BE_ST_SC_S},
	{false,  BE_ST_SC_D},
	{false,  BE_ST_SC_F},
	{false,  BE_ST_SC_G},
	{false,  BE_ST_SC_H},
	{false,  BE_ST_SC_J},
	{false,  BE_ST_SC_K},
	{false,  BE_ST_SC_L},
	{false,  BE_ST_SC_SEMICOLON},
	{false,  BE_ST_SC_QUOTE},
	emptyDOSKeyEvent, // key next to Enter
	emptyDOSKeyEvent,
	{false,  BE_ST_SC_KP_4},
	{false,  BE_ST_SC_KP_5},
	{false,  BE_ST_SC_KP_6},
	{false,  BE_ST_SC_LESSTHAN},
	{false,  BE_ST_SC_Z},
	{false,  BE_ST_SC_X},
	{false,  BE_ST_SC_C},
	{false,  BE_ST_SC_V},
	{false,  BE_ST_SC_B},
	{false,  BE_ST_SC_N},
	{false,  BE_ST_SC_M},
	{false,  BE_ST_SC_COMMA},
	{false,  BE_ST_SC_PERIOD},
	{false,  BE_ST_SC_SLASH},
	emptyDOSKeyEvent,
	{false,  BE_ST_SC_KP_PERIOD},
	{false,  BE_ST_SC_KP_7}, //{true,  BE_ST_SC_HOME},
	{false,  BE_ST_SC_KP_8},
	{false,  BE_ST_SC_KP_9}, //{true,  BE_ST_SC_PAGEUP},
	{false,  BE_ST_SC_SPACE},
	{false,  BE_ST_SC_BSPACE},
	{false,  BE_ST_SC_TAB},
	{true,  BE_ST_SC_KP_ENTER},
	{false,  BE_ST_SC_ENTER},
	{false,  BE_ST_SC_ESC},
	{true,  BE_ST_SC_DELETE},
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	{false,  BE_ST_SC_KP_MINUS},
	emptyDOSKeyEvent,
	{true,  BE_ST_SC_UP},
	{true,  BE_ST_SC_DOWN},
	{true,  BE_ST_SC_RIGHT},
	{true,  BE_ST_SC_LEFT},
	{false,  BE_ST_SC_F1},
	{false,  BE_ST_SC_F2},
	{false,  BE_ST_SC_F3},
	{false,  BE_ST_SC_F4},
	{false,  BE_ST_SC_F5},
	{false,  BE_ST_SC_F6},
	{false,  BE_ST_SC_F7},
	{false,  BE_ST_SC_F8},
	{false,  BE_ST_SC_F9},
	{false,  BE_ST_SC_F10},
	{false,  BE_ST_SC_NUMLOCK},
	{false,  BE_ST_SC_SCROLLLOCK},
	{true,  BE_ST_SC_KP_DIVIDE},
	{false,  BE_ST_SC_KP_MULTIPLY}, //{false,  BE_ST_SC_PRINTSCREEN},
	{false,  BE_ST_SC_KP_PLUS},
	{false,  BE_ST_SC_PAUSE}, // Help key
	{false,  BE_ST_SC_LSHIFT},
	{false,  BE_ST_SC_RSHIFT},
	{false,  BE_ST_SC_CAPSLOCK},
	{false,  BE_ST_SC_LCTRL},
	{false,  BE_ST_SC_LALT},
	{true,  BE_ST_SC_RALT},
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	emptyDOSKeyEvent,
	// no Amiga equivalents
	//{false,  BE_ST_SC_F11},
	//{false,  BE_ST_SC_F12},
	//{true,  BE_ST_SC_RCTRL},
};


static void BEL_ST_HandleEmuKeyboardEvent(bool isPressed, emulatedDOSKeyEvent keyEvent)
{
	if (keyEvent.dosScanCode == BE_ST_SC_PAUSE)
	{
		if (isPressed && g_sdlKeyboardInterruptFuncPtr)
		{
			// SPECIAL: 6 scancodes sent on key press ONLY
			g_sdlKeyboardInterruptFuncPtr(0xe1);
			g_sdlKeyboardInterruptFuncPtr(0x1d);
			g_sdlKeyboardInterruptFuncPtr(0x45);
			g_sdlKeyboardInterruptFuncPtr(0xe1);
			g_sdlKeyboardInterruptFuncPtr(0x9d);
			g_sdlKeyboardInterruptFuncPtr(0xc5);
		}
	}
	else
	{
		if (g_sdlKeyboardInterruptFuncPtr)
		{
			if (keyEvent.isSpecial)
			{
				g_sdlKeyboardInterruptFuncPtr(0xe0);
			}
			g_sdlKeyboardInterruptFuncPtr(keyEvent.dosScanCode | (isPressed ? 0 : 0x80));
		}
		else if (isPressed)
		{
			g_sdlLastKeyScanCode = keyEvent.dosScanCode;
		}
	}
}

#ifdef __MORPHOS__
static struct InputEvent *IN_KeyboardHandlerFunc(void);
static struct EmulLibEntry BEL_ST_InputHandler =
{
	TRAP_LIB, 0, (void (*)(void))IN_KeyboardHandlerFunc
};

static struct InputEvent *IN_KeyboardHandlerFunc()
{
	struct InputEvent *moo = (struct InputEvent *)REG_A0;
	//APTR id = (APTR)REG_A1;
#elif defined(__AMIGA__)
static struct InputEvent * __saveds BEL_ST_InputHandler(register struct InputEvent *moo __asm("a0"), register APTR id __asm("a1"))
{
#else
AROS_UFH2(struct InputEvent *, BEL_ST_InputHandler, AROS_UFHA(struct InputEvent *, moo, A0), AROS_UFHA(APTR, id, A1))
{
	AROS_USERFUNC_INIT
#endif
	struct InputEvent *coin;
	int scanCode, isPressed;

	if (g_amigaScreen != IntuitionBase->FirstScreen)
		return moo;

	coin = moo;

	do
	{
		isPressed = !(coin->ie_Code & IECODE_UP_PREFIX);
		scanCode = coin->ie_Code & ~IECODE_UP_PREFIX;

		// keyboard
		if (coin->ie_Class == IECLASS_RAWKEY)
		{
			BEL_ST_HandleEmuKeyboardEvent(isPressed, sdlKeyMappings[scanCode]);
		}

		// mouse
		if ((coin->ie_Class == IECLASS_RAWMOUSE))
		{
			if (coin->ie_Code != IECODE_NOBUTTON)
			{
				switch (scanCode)
				{
					case IECODE_LBUTTON:
						if (isPressed)
							g_mouseButtonState |= 1;
						else
							g_mouseButtonState &= ~1;
						break;
					case IECODE_RBUTTON:
						if (isPressed)
							g_mouseButtonState |= 2;
						else
							g_mouseButtonState &= ~2;
						break;
				}
			}
			g_mx += coin->ie_position.ie_xy.ie_x;
			g_my += coin->ie_position.ie_xy.ie_y;
		}

		coin = coin->ie_NextEvent;
	} while (coin);

	return moo;
#if defined(__AROS__)
	AROS_USERFUNC_EXIT
#endif
}

void BE_ST_InitAll(void)
{
	//g_pointerMem = (UWORD *)AllocVec(16 * 16, MEMF_CLEAR | MEMF_CHIP);

	if ((g_inputPort = CreateMsgPort()))
	{
		if ((g_inputReq = (struct IOStdReq *)CreateIORequest(g_inputPort, sizeof(*g_inputReq))))
		{
			if (!OpenDevice((STRPTR)"input.device", 0, (struct IORequest *)g_inputReq, 0))
			{
				g_inputHandler.is_Node.ln_Type = NT_INTERRUPT;
				g_inputHandler.is_Node.ln_Pri = 16;
				g_inputHandler.is_Node.ln_Name = (STRPTR)"Catacomb 3-D input handler";
				g_inputHandler.is_Code = (void (*)())&BEL_ST_InputHandler;
				g_inputReq->io_Data = (void *)&g_inputHandler;
				g_inputReq->io_Command = IND_ADDHANDLER;
				if (!DoIO((struct IORequest *)g_inputReq))
				{
					BE_ST_InitGfx();
					BE_ST_InitAudio();
					//BE_ST_PollEvents(); // e.g., to "reset" some states, and detect joysticks
					return;
				}
			}
		}
	}

	BE_ST_ShutdownAll();
}

void BE_ST_ShutdownAll(void)
{
	if (g_inputReq)
	{
		g_inputReq->io_Data = (void *)&g_inputHandler;
		g_inputReq->io_Command = IND_REMHANDLER;
		DoIO((struct IORequest *)g_inputReq);
		CloseDevice((struct IORequest *)g_inputReq);
		DeleteIORequest((struct IORequest *)g_inputReq);
		g_inputReq = NULL;
	}

	if (g_inputPort)
	{
		DeleteMsgPort(g_inputPort);
		g_inputPort = NULL;
	}

	/*if (g_pointerMem)
	{
		ClearPointer(awindow);
		FreeVec(g_pointerMem);
		g_pointerMem = NULL;
	}*/

	BE_ST_ShutdownAudio();
	BE_ST_ShutdownGfx();
}

void BE_ST_HandleExit(int status)
{
	BE_ST_ShutdownAll();
	exit(0);
}

void BE_ST_ExitWithErrorMsg(const char *msg)
{
	BE_ST_SetScreenMode(3);
	BE_ST_puts(msg);
	BE_Cross_LogMessage(BE_LOG_MSG_ERROR, "%s\n", msg);
	BE_ST_HandleExit(1);
}

void BE_ST_StartKeyboardService(void (*funcPtr)(uint8_t))
{
	g_sdlKeyboardInterruptFuncPtr = funcPtr;
}

void BE_ST_StopKeyboardService(void)
{
	g_sdlKeyboardInterruptFuncPtr = 0;
}

void BE_ST_GetMouseDelta(int16_t *x, int16_t *y)
{
	if (x)
	{
		*x = g_mx;
	}
	if (y)
	{
		*y = g_my;
	}

	g_mx = g_my = 0;
}

uint16_t BE_ST_GetMouseButtons(void)
{
	return g_mouseButtonState;
}


void BE_ST_GetJoyAbs(uint16_t joy, uint16_t *xp, uint16_t *yp)
{
	uint16_t jx = 0;
	uint16_t jy = 0;
	ULONG portstate;

	D(bug("%s(%u)\n", __FUNCTION__, joy));

	portstate = ReadJoyPort(joy);

	if (portstate & (JP_TYPE_GAMECTLR | JP_TYPE_JOYSTK))
	{
		jx = jy = 1; // center for the joystick detection

		if (portstate & JPF_JOY_UP)
			jy = 0;
		else if (portstate & JPF_JOY_DOWN)
			jy = 2;

		if (portstate & JPF_JOY_LEFT)
			jx = 0;
		else if (portstate & JPF_JOY_RIGHT)
			jx = 2;

		D(bug("jx %d jy %d\n", jx, jy));
	}

	*xp = jx;
	*yp = jy;
}

uint16_t BE_ST_GetJoyButtons(uint16_t joy)
{
	uint16_t result = 0;
	ULONG portstate;

	D(bug("%s(%u)\n", __FUNCTION__, joy));

	portstate = ReadJoyPort(joy);

	if (portstate & (JP_TYPE_GAMECTLR | JP_TYPE_JOYSTK))
	{
		if (portstate & JPF_BUTTON_BLUE) result |= 1 << 0;
		if (portstate & JPF_BUTTON_RED) result |= 1 << 1;
		/*if (portstate & JPF_BUTTON_YELLOW) result |= 1 << 2;
		if (portstate & JPF_BUTTON_GREEN) result |= 1 << 3;
		if (portstate & JPF_BUTTON_FORWARD) result |= 1 << 4;
		if (portstate & JPF_BUTTON_REVERSE) result |= 1 << 5;
		if (portstate & JPF_BUTTON_PLAY) result |= 1 << 6;*/
		D(bug("buttons %u\n", result));
	}

	return result;
}

int16_t BE_ST_KbHit(void)
{
	return g_sdlLastKeyScanCode;
}

int16_t BE_ST_BiosScanCode(int16_t command)
{
	if (command == 1)
	{
		return g_sdlLastKeyScanCode;
	}

	while (!g_sdlLastKeyScanCode)
	{
		BE_ST_ShortSleep();
	}
	int16_t result = g_sdlLastKeyScanCode;
	g_sdlLastKeyScanCode = 0;
	return result;
}


void BE_ST_AltControlScheme_Push(void)
{
}

void BE_ST_AltControlScheme_Pop(void)
{
}


void BE_ST_AltControlScheme_PrepareFaceButtonsDOSScancodes(const char *scanCodes)
{
}

void BE_ST_AltControlScheme_PreparePageScrollingControls(int prevPageScan, int nextPageScan)
{
}

void BE_ST_AltControlScheme_PrepareMenuControls(void)
{
}

void BE_ST_AltControlScheme_PrepareInGameControls(int primaryScanCode, int secondaryScanCode, int upScanCode, int downScanCode, int leftScanCode, int rightScanCode)
{

}

void BE_ST_AltControlScheme_PrepareInputWaitControls(void)
{
}

void BE_ST_AltControlScheme_PrepareTextInput(void)
{
}


void BE_ST_PollEvents(void)
{
	//BE_ST_PrepareForManualAudioSDServiceCall();
}

#ifdef BE_ST_ENABLE_FARPTR_CFG
uint16_t BE_ST_Compat_GetFarPtrRelocationSegOffset(void)
{
	return BE_ST_DEFAULT_FARPTRSEGOFFSET;
}
#endif
