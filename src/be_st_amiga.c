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
#define USE_INPUT_DEVICE

// 16KB stack minimum
unsigned long __attribute__((used)) __stack=0x4000;

RefKeenConfig g_refKeenCfg;
BE_ST_ControllerMapping g_beStControllerMappingTextInput;
BE_ST_ControllerMapping g_beStControllerMappingDebugKeys;

// mapping from CD32 to SDL2 buttons
ULONG g_CD32Buttons[BE_ST_CTRL_BUT_MAX] = 
{
	JPF_BUTTON_BLUE,	// BE_ST_CTRL_BUT_A
	JPF_BUTTON_RED,		// BE_ST_CTRL_BUT_B
	JPF_BUTTON_YELLOW,	// BE_ST_CTRL_BUT_X
	JPF_BUTTON_GREEN,	// BE_ST_CTRL_BUT_Y
	JPF_BUTTON_PLAY/*0*/,					// BE_ST_CTRL_BUT_BACK
	0,					// BE_ST_CTRL_BUT_GUIDE
	0/*JPF_BUTTON_PLAY*/,	// BE_ST_CTRL_BUT_START
	0,					// BE_ST_CTRL_BUT_LSTICK
	0,					// BE_ST_CTRL_BUT_RSTICK
	JPF_BUTTON_REVERSE,	// BE_ST_CTRL_BUT_LSHOULDER
	JPF_BUTTON_FORWARD,	// BE_ST_CTRL_BUT_RSHOULDER
	JPF_JOY_UP,			// BE_ST_CTRL_BUT_DPAD_UP
	JPF_JOY_DOWN,		// BE_ST_CTRL_BUT_DPAD_DOWN
	JPF_JOY_LEFT,		// BE_ST_CTRL_BUT_DPAD_LEFT
	JPF_JOY_RIGHT,		// BE_ST_CTRL_BUT_DPAD_RIGHT
};

/*** These represent button states, although a call to BEL_ST_AltControlScheme_CleanUp zeros these out ***/
static bool g_sdlControllersButtonsStates[BE_ST_CTRL_BUT_MAX];

#define NUM_OF_CONTROLLER_MAPS_IN_STACK 8

static bool g_sdlControllerSchemeNeedsCleanUp;

static struct {
	const BE_ST_ControllerMapping *stack[NUM_OF_CONTROLLER_MAPS_IN_STACK];
	const BE_ST_ControllerMapping **currPtr;
	const BE_ST_ControllerMapping **endPtr;
} g_sdlControllerMappingPtrsStack;

// Current mapping, doesn't have to be *(g_sdlControllerMappingPtrsStack.currPtr) as game code can change this (e.g., helper keys)
const BE_ST_ControllerMapping *g_sdlControllerMappingActualCurr;

static BE_ST_ControllerMapping g_sdlControllerMappingDefault;

static void (*g_sdlKeyboardInterruptFuncPtr)(uint8_t) = 0;
static void (*g_sdlAppQuitCallback)(void) = 0;
extern struct Screen *g_amigaScreen;

uint8_t g_sdlLastKeyScanCode;
uint16_t g_mouseButtonState = 0;
int16_t g_mx = 0;
int16_t g_my = 0;

#ifdef USE_INPUT_DEVICE
static struct Interrupt g_inputHandler;
static struct MsgPort *g_inputPort = NULL;
static struct IOStdReq *g_inputReq = NULL;
#else
static APTR g_kbdIntHandle = NULL;
static int g_oldMouseX = 0;
static int g_oldMouseY = 0;
#endif
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

#ifdef USE_INPUT_DEVICE
static struct InputEvent * __saveds BEL_ST_InputHandler(register struct InputEvent *moo __asm("a0"), register APTR id __asm("a1"))
{
	struct InputEvent *coin;
	int scanCode, isPressed;

	if (g_amigaScreen != IntuitionBase->FirstScreen)
		return moo;

	for (coin = moo; coin; coin = coin->ie_NextEvent)
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
	}

	return moo;
}
#else
static void __interrupt __saveds BEL_ST_KeyboardInterrupt(register UBYTE rawKey __asm("d0"), register APTR intData __asm("a1"))
{
	int scanCode, isPressed;

	isPressed = !(rawKey & IECODE_UP_PREFIX);
	scanCode = rawKey & ~IECODE_UP_PREFIX;

	BEL_ST_HandleEmuKeyboardEvent(isPressed, sdlKeyMappings[scanCode]);

}
#endif

static void BEL_ST_ParseConfig(void)
{
	// Defaults
//#ifdef REFKEEN_VER_CATACOMB_ALL
	g_refKeenCfg.altControlScheme.isEnabled = true;
/*#else
	g_refKeenCfg.altControlScheme.isEnabled = false;
#endif*/

#ifdef REFKEEN_VER_KDREAMS
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_JUMP] = BE_ST_CTRL_BUT_A;
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_THROW] = BE_ST_CTRL_BUT_B;
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_STATS] = BE_ST_CTRL_BUT_X;
#else
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_FIRE] = BE_ST_CTRL_BUT_A; // BE_ST_CTRL_BUT_LSHOULDER
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_STRAFE] = BE_ST_CTRL_BUT_B;
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_DRINK] = BE_ST_CTRL_BUT_LSHOULDER; // BE_ST_CTRL_BUT_A
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_BOLT] = BE_ST_CTRL_BUT_X;
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_NUKE] = BE_ST_CTRL_BUT_Y;
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_FASTTURN] = BE_ST_CTRL_BUT_RSHOULDER;
#endif
#if (defined REFKEEN_VER_CAT3D) || (defined REFKEEN_VER_CATABYSS)
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_SCROLLS] = BE_ST_CTRL_BUT_MAX+1; // HACK for getting right trigger (technically an axis)
#endif
#if (defined REFKEEN_VER_KDREAMS) || (defined REFKEEN_VER_CATADVENTURES)
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_FUNCKEYS] = BE_ST_CTRL_BUT_MAX; // HACK for getting left trigger (technically an axis)
#endif
	g_refKeenCfg.altControlScheme.actionMappings[BE_ST_CTRL_CFG_BUTMAP_DEBUGKEYS] = BE_ST_CTRL_BUT_LSTICK;

	g_refKeenCfg.altControlScheme.useDpad = true;
	g_refKeenCfg.altControlScheme.useLeftStick = false;
	g_refKeenCfg.altControlScheme.useRightStick = false;
//#ifdef REFKEEN_VER_CATACOMB_ALL
	g_refKeenCfg.altControlScheme.analogMotion = false;
//#endif
	//g_refKeenCfg.manualGameVerMode = false;
}

void BE_ST_InitCommon(void)
{
	// MUST be called BEFORE parsing config (of course!)
	BE_Cross_PrepareAppPaths();

	BEL_ST_ParseConfig();
}

#ifdef USE_INPUT_DEVICE
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
#endif

static bool BEL_ST_InitInput(void)
{
#ifdef USE_INPUT_DEVICE
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
					return true;
				}
			}
		}
	}
#else
	g_mousePort = 0;
	BE_ST_GetEmuAccuMouseMotion(NULL, NULL);
	if ((g_kbdIntHandle = AddKBInt(BEL_ST_KeyboardInterrupt, NULL)))
	{
		return true;
	}
#endif

	return false;
}

static void BEL_ST_ShutdownInput(void)
{
#ifdef USE_INPUT_DEVICE
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
#else
	if (g_kbdIntHandle)
	{
		RemKBInt(g_kbdIntHandle);
		g_kbdIntHandle = NULL;
	}
#endif
}

void BE_ST_PrepareForGameStartupWithoutAudio(void)
{
#ifndef USE_INPUT_DEVICE
	SystemControl(
		SCON_TakeOverSys, TRUE,
		//SCON_StopInput, TRUE,
		//SCON_KillReq, TRUE,
		TAG_DONE);
#endif

	if (BEL_ST_InitInput())
	{
		BE_ST_InitGfx();
		//BE_ST_InitAudio();

		// Preparing a controller scheme (with no special UI) in case the relevant feature is enabled
		g_sdlControllerMappingPtrsStack.stack[0] = &g_sdlControllerMappingDefault;
		g_sdlControllerMappingPtrsStack.currPtr = &g_sdlControllerMappingPtrsStack.stack[0];
		g_sdlControllerMappingPtrsStack.endPtr = &g_sdlControllerMappingPtrsStack.stack[NUM_OF_CONTROLLER_MAPS_IN_STACK];
		g_sdlControllerMappingActualCurr = g_sdlControllerMappingPtrsStack.stack[0];

		g_sdlControllerSchemeNeedsCleanUp = false;

		memset(g_sdlControllersButtonsStates, 0, sizeof(g_sdlControllersButtonsStates));

		// Then use this to reset/update some states, and detect joysticks
		//BE_ST_PollEvents();

		return;
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
	BEL_ST_ShutdownInput();
	BE_ST_ShutdownAudio();
	BE_ST_ShutdownGfx();

#ifndef USE_INPUT_DEVICE
	SystemControl(SCON_TakeOverSys, FALSE, TAG_DONE);
#endif
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

void BE_ST_GetEmuAccuMouseMotion(int16_t *x, int16_t *y)
{
#ifndef USE_INPUT_DEVICE
	int newMouseX, newMouseY;
	ULONG portstate;

	portstate = ReadJoyPort(g_mousePort);

	newMouseX = (portstate & JP_MHORZ_MASK);
	newMouseY = ((portstate & JP_MVERT_MASK) >> 8);
	g_mx = newMouseX - g_oldMouseX;
	g_my = newMouseY - g_oldMouseY;

	// check for underflow/overflow
	if (g_mx < -127)
		g_mx = g_mx + 256;
	else if (g_mx > 127)
		g_mx = g_mx - 256;

	if (g_my < -127)
		g_my = g_my + 256;
	else if (g_my > 127)
		g_my = g_my - 256;

	g_oldMouseX = newMouseX;
	g_oldMouseY = newMouseY;
#endif

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

uint16_t BE_ST_GetEmuMouseButtons(void)
{
#ifndef USE_INPUT_DEVICE
	ULONG portstate;

	portstate = ReadJoyPort(g_mousePort);

	g_mouseButtonState = 0;
	if (portstate & JPF_BUTTON_RED)
		g_mouseButtonState |= 1 << 0; // Left mouse
	if (portstate & JPF_BUTTON_BLUE)
		g_mouseButtonState |= 1 << 1; // Right mouse
#endif

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
void BE_ST_GetEmuJoyAxes(uint16_t joy, uint16_t *xp, uint16_t *yp)
{
	uint16_t jx = JOYSTICK_MIN;
	uint16_t jy = JOYSTICK_MIN;
	ULONG portstate;

	D(bug("%s(%u)\n", __FUNCTION__, joy));

	joy = BEL_ST_PortIndex(joy);
//#ifdef MOUSE_KLUDGE
	if (g_refKeenCfg.altControlScheme.isEnabled || joy == g_mousePort)
	{
		*xp = JOYSTICK_MIN;
		*yp = JOYSTICK_MIN;
		return;
	}
//#endif

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

		//BE_ST_DebugText(0, 0, "joy %d portstate %08x jx %d jy %d", joy, portstate, jx, jy);
		D(bug("joy %d portstate %08x jx %d jy %d\n", joy, portstate, jx, jy));
	}

	*xp = jx;
	*yp = jy;
}

uint16_t BE_ST_GetEmuJoyButtons(uint16_t joy)
{
	uint16_t result = 0;
	ULONG portstate;

	D(bug("%s(%u)\n", __FUNCTION__, joy));

	joy = BEL_ST_PortIndex(joy);
//#ifdef MOUSE_KLUDGE
	if (g_refKeenCfg.altControlScheme.isEnabled || joy == g_mousePort)
		return 0;
//#endif

	//portstate = ReadJoyPort(BEL_ST_PortIndex(joy));
	portstate = ReadJoyPort(joy);

	if (((portstate & JP_TYPE_MASK) == JP_TYPE_GAMECTLR) || ((portstate & JP_TYPE_MASK) == JP_TYPE_JOYSTK))
	{
		if (portstate & JPF_BUTTON_BLUE) result |= 1 << 0;
		if (portstate & JPF_BUTTON_RED) result |= 1 << 1;
		/*if (portstate & JPF_BUTTON_YELLOW) result |= 1 << 2;
		if (portstate & JPF_BUTTON_GREEN) result |= 1 << 3;
		if (portstate & JPF_BUTTON_FORWARD) result |= 1 << 4;
		if (portstate & JPF_BUTTON_REVERSE) result |= 1 << 5;
		if (portstate & JPF_BUTTON_PLAY) result |= 1 << 6;*/

		//BE_ST_DebugText(0, 8, "joy %d portstate %08x buttons %u", joy, portstate, result);
		D(bug("joy %d portstate %08x buttons %u\n", joy, portstate, result));
	}

	return result;
}

int16_t BE_ST_KbHit(void)
{
	BE_ST_PollEvents();
	return g_sdlLastKeyScanCode;
}

int16_t BE_ST_BiosScanCode(int16_t command)
{
	BE_ST_PollEvents();

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


static void BEL_ST_AltControlScheme_CleanUp(void);

// May be similar to PrepareControllerMapping, but a bit different:
// Used in order to replace controller mapping with another one internally
// (e.g., showing helper function keys during gameplay, or hiding such keys)
void BEL_ST_ReplaceControllerMapping(const BE_ST_ControllerMapping *mapping)
{
	BEL_ST_AltControlScheme_CleanUp();
	g_sdlControllerMappingActualCurr = mapping;

	g_sdlControllerSchemeNeedsCleanUp = true;
}

bool BEL_ST_AltControlScheme_HandleEntry(const BE_ST_ControllerSingleMap *map, int value, bool *lastBinaryStatusPtr)
{
	bool prevBinaryStatus = *lastBinaryStatusPtr;
	*lastBinaryStatusPtr = (value /*>= g_sdlJoystickAxisBinaryThreshold*/);
	switch (map->mapClass)
	{
	case BE_ST_CTRL_MAP_KEYSCANCODE:
	{
		if (*lastBinaryStatusPtr != prevBinaryStatus)
		{
			emulatedDOSKeyEvent dosKeyEvent;
			dosKeyEvent.isSpecial = false;
			dosKeyEvent.dosScanCode = map->val;
			BEL_ST_HandleEmuKeyboardEvent(*lastBinaryStatusPtr, /*false,*/ dosKeyEvent);
		}
		return true;
	}
	case BE_ST_CTRL_MAP_OTHERMAPPING:
		if (!prevBinaryStatus && (*lastBinaryStatusPtr))
			BEL_ST_ReplaceControllerMapping(map->otherMappingPtr);
		return true; // Confirm either way
	}
	return false;
}

/* WARNING: In theory there may be a Clear -> HandleEntry -> Clear cycle,
 * but it can never occur since isPressed is set to false
 */
static void BEL_ST_AltControlScheme_ClearBinaryStates(void)
{
	/*if (g_sdlControllerMappingActualCurr == &g_beStControllerMappingTextInput)
	{
		BEL_ST_ReleasePressedKeysInTextInputUI();
	}
	else if (g_sdlControllerMappingActualCurr == &g_beStControllerMappingDebugKeys)
	{
		BEL_ST_ReleasePressedKeysInDebugKeysUI();
	}
	else*/ // Otherwise simulate key releases based on the mapping
	{
		// Simulate binary key/button/other action "releases" and clear button states.
		// FIXME: Unfortunately this means a mistaken key release event can be sent, but hopefully it's less of an issue than an unexpected key press.
		for (int but = 0; but < BE_ST_CTRL_BUT_MAX; ++but)
		{
			BEL_ST_AltControlScheme_HandleEntry(&g_sdlControllerMappingActualCurr->buttons[but], 0, &g_sdlControllersButtonsStates[but]);
		}
	}

	memset(g_sdlControllersButtonsStates, 0, sizeof(g_sdlControllersButtonsStates));
}

static void BEL_ST_AltControlScheme_CleanUp(void)
{
	if (!g_sdlControllerSchemeNeedsCleanUp)
		return;

	BEL_ST_AltControlScheme_ClearBinaryStates();

	g_sdlControllerSchemeNeedsCleanUp = false;
}

void BE_ST_AltControlScheme_Push(void)
{
	if (!g_refKeenCfg.altControlScheme.isEnabled)
		return;

	BEL_ST_AltControlScheme_CleanUp();

	++g_sdlControllerMappingPtrsStack.currPtr;
	if (g_sdlControllerMappingPtrsStack.currPtr == g_sdlControllerMappingPtrsStack.endPtr)
	{
		BE_ST_ExitWithErrorMsg("BE_ST_AltControlScheme_Push: Out of stack bounds!\n");
	}
}

void BE_ST_AltControlScheme_Pop(void)
{
	if (!g_refKeenCfg.altControlScheme.isEnabled)
		return;

	BEL_ST_AltControlScheme_CleanUp();

	if (g_sdlControllerMappingPtrsStack.currPtr == &g_sdlControllerMappingPtrsStack.stack[0])
	{
		BE_ST_ExitWithErrorMsg("BE_ST_AltControlScheme_Pop: Popped more than necessary!\n");
	}
	--g_sdlControllerMappingPtrsStack.currPtr;

	g_sdlControllerMappingActualCurr = *g_sdlControllerMappingPtrsStack.currPtr;

	g_sdlControllerSchemeNeedsCleanUp = true;
}

void BE_ST_AltControlScheme_PrepareControllerMapping(const BE_ST_ControllerMapping *mapping)
{
	if (!g_refKeenCfg.altControlScheme.isEnabled)
		return;

	BEL_ST_AltControlScheme_CleanUp();
	g_sdlControllerMappingActualCurr = *g_sdlControllerMappingPtrsStack.currPtr = mapping;

	g_sdlControllerSchemeNeedsCleanUp = true;
}

void BE_ST_AltControlScheme_UpdateVirtualMouseCursor(int x, int y)
{
	// unused
}

void BE_ST_AltControlScheme_Reset(void)
{
	BEL_ST_AltControlScheme_CleanUp();

	g_sdlControllerMappingPtrsStack.stack[0] = &g_sdlControllerMappingDefault;
	g_sdlControllerMappingPtrsStack.currPtr = &g_sdlControllerMappingPtrsStack.stack[0];
	g_sdlControllerMappingPtrsStack.endPtr = &g_sdlControllerMappingPtrsStack.stack[NUM_OF_CONTROLLER_MAPS_IN_STACK];
	g_sdlControllerMappingActualCurr = g_sdlControllerMappingPtrsStack.stack[0];

	g_sdlControllerSchemeNeedsCleanUp = true;
}

void BE_ST_SetAppQuitCallback(void (*funcPtr)(void))
{
	g_sdlAppQuitCallback = funcPtr;
}

void BE_ST_PollEvents(void)
{
	ULONG portstate;

	if (!g_refKeenCfg.altControlScheme.isEnabled)
		return;

	portstate = ReadJoyPort(1);

	//BE_ST_DebugText(0, 0, "joy %d portstate %08x", 1, portstate);
	for (int but = 0; but < BE_ST_CTRL_BUT_MAX; ++but)
	{
		bool isPressed = portstate & g_CD32Buttons[but];
		BEL_ST_AltControlScheme_HandleEntry(&g_sdlControllerMappingActualCurr->buttons[but], isPressed, &g_sdlControllersButtonsStates[but]);
	}
}

#ifdef BE_ST_ENABLE_FARPTR_CFG
uint16_t BE_ST_Compat_GetFarPtrRelocationSegOffset(void)
{
	return BE_ST_DEFAULT_FARPTRSEGOFFSET;
}
#endif
