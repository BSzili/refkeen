#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "SDL.h"
#include <libraries/lowlevel.h>
#include <devices/input.h>
#include <devices/gameport.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <exec/interrupts.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include "debug_amiga.h"

#include "be_cross.h"
#include "be_st.h"
#include "be_gamever.h"

#define BE_ST_MAXJOYSTICKS 8
#define BE_ST_EMU_JOYSTICK_RANGEMAX 5000 // id_in.c MaxJoyValue
#define BE_ST_DEFAULT_FARPTRSEGOFFSET 0x14

#if (defined REFKEEN_VER_CATARM) || (defined REFKEEN_VER_CATAPOC)
#define BE_ST_ENABLE_FARPTR_CFG 1
#endif

#if defined(__AMIGA__)
#define IPTR ULONG
#endif

#define MOUSE_KLUDGE

// 16KB stack minimum
unsigned long __attribute__((used)) __stack=0x4000;

// TODO: fix these
RefKeenConfig g_refKeenCfg;
BE_ST_ControllerMapping g_beStControllerMappingTextInput;
BE_ST_ControllerMapping g_beStControllerMappingDebugKeys;
// more alt controller stubs from altcontroller.c
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_inackback;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_menu_help;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_menu;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_notenoughmemorytostart;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_simpledialog;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_menu_confirm;
#ifdef REFKEEN_VER_CATACOMB_ALL
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_menu_paddle;
#endif
#ifdef REFKEEN_VER_CATADVENTURES
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_soundoptions;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_keychoice;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_help;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_waitforspace;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_intro;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_intro_skillselection;
BE_ST_ControllerMapping g_ingame_altcontrol_mapping_saveoverwriteconfirm;
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
static int g_mousePort = -1;

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
	emptyDOSKeyEvent, // international key next to Enter
	emptyDOSKeyEvent,
	{false,  BE_ST_SC_KP_4},
	{false,  BE_ST_SC_KP_5},
	{false,  BE_ST_SC_KP_6},
	{false,  BE_ST_SC_LESSTHAN}, // international key between the left Shift and Z
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

static struct InputEvent * __saveds BEL_ST_InputHandler(register struct InputEvent *moo __asm("a0"), register APTR id __asm("a1"))
{
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
}

void BE_ST_InitCommon(void)
{
	// MUST be called BEFORE parsing config (of course!)
	BE_Cross_PrepareAppPaths();

	//BEL_ST_ParseConfig();
}

static void BEL_ST_CheckMouse(struct MsgPort *ioReplyPort, ULONG unit)
{
	struct IOStdReq *gameport_io;
	BYTE gameport_ct;

	if ((gameport_io = (struct IOStdReq *)CreateIORequest(ioReplyPort, sizeof(struct IOStdReq))))
	{
		if (!OpenDevice("gameport.device", unit, (struct IORequest *)gameport_io, 0))
		{
			gameport_io->io_Command = GPD_ASKCTYPE;
			gameport_io->io_Length = 1;
			gameport_io->io_Data = &gameport_ct;
			DoIO((struct IORequest *)gameport_io);
			if (gameport_ct == GPCT_MOUSE)
			{
				D(bug("mouse is in port %lu\n", unit));
				g_mousePort = unit;
			}
			CloseDevice((struct IORequest *)gameport_io);
		}
		DeleteIORequest((struct IORequest *)gameport_io);
	}
}

void BE_ST_PrepareForGameStartup(void)
{
	if ((g_inputPort = CreateMsgPort()))
	{
#ifdef MOUSE_KLUDGE
		BEL_ST_CheckMouse(g_inputPort, 0);
		BEL_ST_CheckMouse(g_inputPort, 1);
#endif
		if ((g_inputReq = (struct IOStdReq *)CreateIORequest(g_inputPort, sizeof(*g_inputReq))))
		{
			if (!OpenDevice((STRPTR)"input.device", 0, (struct IORequest *)g_inputReq, 0))
			{
				g_inputHandler.is_Node.ln_Type = NT_INTERRUPT;
				g_inputHandler.is_Node.ln_Pri = 100;
				g_inputHandler.is_Node.ln_Name = (STRPTR)"Catacomb 3-D input handler";
				g_inputHandler.is_Code = (void (*)())&BEL_ST_InputHandler;
				g_inputReq->io_Data = (void *)&g_inputHandler;
				g_inputReq->io_Command = IND_ADDHANDLER;
				if (!DoIO((struct IORequest *)g_inputReq))
				{
					BE_ST_InitGfx();
					BE_ST_InitAudio();
					return;
				}
			}
		}
	}

	//BE_ST_ShutdownAll();
	BE_ST_QuickExit();
}

/*void __chkabort(void)
{
#ifdef DEBUG
	BE_ST_ShutdownAll();
#endif
}*/

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

	BE_ST_ShutdownAudio();
	BE_ST_ShutdownGfx();
}

void BE_ST_HandleExit(int status)
{
#ifdef REFKEEN_VER_CATABYSS
	if (refkeen_current_gamever == BE_GAMEVER_CATABYSS113)
		BE_ST_BiosScanCode(0);
#endif
#ifdef REFKEEN_VER_KDREAMS
	if (refkeen_current_gamever == BE_GAMEVER_KDREAMSE113)
		BE_ST_BiosScanCode(0);
#endif
	BE_ST_ShutdownAll();
	exit(status);
}

void BE_ST_ExitWithErrorMsg(const char *msg)
{
	/*BE_ST_SetScreenMode(3);
	BE_ST_puts(msg);*/
	BE_Cross_LogMessage(BE_LOG_MSG_ERROR, "%s\n", msg);
	BE_ST_HandleExit(1);
}

void BE_ST_QuickExit(void)
{
	//BEL_ST_SaveConfig();
	BE_ST_ShutdownAll();
	exit(0);
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


static ULONG BEL_ST_PortIndex(uint16_t joy)
{
	switch (joy)
	{
		case 0:
			return 1;
		case 1:
			return 0;
	}

	return joy;
}

#define JOYSTICK_MIN 0
#define JOYSTICK_MAX 2
//#define JOYSTICK_MAX BE_ST_EMU_JOYSTICK_RANGEMAX
#define JOYSTICK_CENTER ((JOYSTICK_MAX-JOYSTICK_MIN)/2)
void BE_ST_DebugText(int x, int y, const char *fmt, ...);
void BE_ST_GetJoyAbs(uint16_t joy, uint16_t *xp, uint16_t *yp)
{
	uint16_t jx = JOYSTICK_MIN;
	uint16_t jy = JOYSTICK_MIN;
	ULONG portstate;

	D(bug("%s(%u)\n", __FUNCTION__, joy));

	joy = BEL_ST_PortIndex(joy);
#ifdef MOUSE_KLUDGE
	if (joy == g_mousePort)
	{
		*xp = JOYSTICK_MIN;
		*yp = JOYSTICK_MIN;
		return;
	}
#endif

	//portstate = ReadJoyPort(BEL_ST_PortIndex(joy));
	portstate = ReadJoyPort(joy);

	if (((portstate & JP_TYPE_MASK) != JP_TYPE_NOTAVAIL) /*&& ((portstate & JP_TYPE_MASK) != JP_TYPE_MOUSE)*/)
	{
		jx = jy = JOYSTICK_CENTER; // center for the joystick detection
	}

	if (((portstate & JP_TYPE_MASK) == JP_TYPE_GAMECTLR) || ((portstate & JP_TYPE_MASK) == JP_TYPE_JOYSTK))
	{
		//jx = jy = JOYSTICK_CENTER; // center for the joystick detection

		if (portstate & JPF_JOY_UP)
			jy = JOYSTICK_MIN;
		else if (portstate & JPF_JOY_DOWN)
			jy = JOYSTICK_MAX;

		if (portstate & JPF_JOY_LEFT)
			jx = JOYSTICK_MIN;
		else if (portstate & JPF_JOY_RIGHT)
			jx = JOYSTICK_MAX;

#ifdef REFKEEN_VER_CATACOMB_ALL
		// strafing part 1, set direction
		if (portstate & JPF_BUTTON_FORWARD && jx != JOYSTICK_MIN) jx = JOYSTICK_MAX;
		if (portstate & JPF_BUTTON_REVERSE && jx != JOYSTICK_MAX) jx = JOYSTICK_MIN;
#endif

		//BE_ST_DebugText(0, 0, "joy %d portstate %08x jx %d jy %d", joy, portstate, jx, jy);
		D(bug("joy %d portstate %08x jx %d jy %d\n", joy, portstate, jx, jy));
	}

	*xp = jx;
	*yp = jy;
}

ULONG g_oldportstate;
#define BEL_ST_CheckJoyEvent(scanCode, mask) \
	do { \
		if ((portstate & mask) != (g_oldportstate & mask)) \
			BEL_ST_HandleEmuKeyboardEvent((portstate & mask) ? true : false, sdlKeyMappings[scanCode]); \
	} while(0)

uint16_t BE_ST_GetJoyButtons(uint16_t joy)
{
	uint16_t result = 0;
	ULONG portstate;

	D(bug("%s(%u)\n", __FUNCTION__, joy));

	joy = BEL_ST_PortIndex(joy);
#ifdef MOUSE_KLUDGE
	if (joy == g_mousePort)
		return 0;
#endif

	//portstate = ReadJoyPort(BEL_ST_PortIndex(joy));
	portstate = ReadJoyPort(joy);

	if (((portstate & JP_TYPE_MASK) == JP_TYPE_GAMECTLR) || ((portstate & JP_TYPE_MASK) == JP_TYPE_JOYSTK))
	{
		if (portstate & JPF_BUTTON_BLUE) result |= 1 << 0;
#ifdef REFKEEN_VER_CATACOMB_ALL
		// we have separate strafe buttons on CD32 controllers, use red for running
		if ((portstate & JP_TYPE_MASK) == JP_TYPE_GAMECTLR)
#ifdef REFKEEN_VER_CAT3D
			BEL_ST_CheckJoyEvent(0x61, JPF_BUTTON_RED); // Right Shift - Run
#else
			BEL_ST_CheckJoyEvent(0x42, JPF_BUTTON_RED); // Tab - Quick Turn
#endif
		else
#endif
		if (portstate & JPF_BUTTON_RED) result |= 1 << 1;
		/*if (portstate & JPF_BUTTON_YELLOW) result |= 1 << 2;
		if (portstate & JPF_BUTTON_GREEN) result |= 1 << 3;
		if (portstate & JPF_BUTTON_FORWARD) result |= 1 << 4;
		if (portstate & JPF_BUTTON_REVERSE) result |= 1 << 5;
		if (portstate & JPF_BUTTON_PLAY) result |= 1 << 6;*/
#ifdef REFKEEN_VER_CATACOMB_ALL
		// strafing part 2, set button1
		if ((portstate & JPF_BUTTON_FORWARD) || (portstate & JPF_BUTTON_REVERSE)) result |= 1 << 1;

		// emulate key presses for the rest of the CD32 buttons
#ifdef REFKEEN_VER_CAT3D
		BEL_ST_CheckJoyEvent(0x35, JPF_BUTTON_YELLOW); // B - Bolt
		BEL_ST_CheckJoyEvent(0x36, JPF_BUTTON_GREEN); // N - Nuke
#else
		BEL_ST_CheckJoyEvent(0x31, JPF_BUTTON_YELLOW); // Z - Zapper
		BEL_ST_CheckJoyEvent(0x32, JPF_BUTTON_GREEN); // X - Xterminator
#endif
		BEL_ST_CheckJoyEvent(0x40, JPF_BUTTON_PLAY); // Space - Heal/Cure

		g_oldportstate = portstate;
#endif

		//BE_ST_DebugText(0, 8, "joy %d portstate %08x buttons %u", joy, portstate, result);
		D(bug("joy %d portstate %08x buttons %u\n", joy, portstate, result));
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

void BE_ST_AltControlScheme_PrepareControllerMapping(const BE_ST_ControllerMapping *mapping)
{
}

void BE_ST_AltControlScheme_UpdateVirtualMouseCursor(int x, int y)
{
}

void BE_ST_PollEvents(void)
{
	//BE_ST_PrepareForManualAudioSDServiceCall();
}

void FillControlPanelTouchMappings(bool withmouse)
{
}

void PrepareGamePlayControllerMapping(void)
{
}

void RefKeen_PrepareAltControllerScheme(void)
{
}

#ifdef BE_ST_ENABLE_FARPTR_CFG
uint16_t BE_ST_Compat_GetFarPtrRelocationSegOffset(void)
{
	return BE_ST_DEFAULT_FARPTRSEGOFFSET;
}
#endif
