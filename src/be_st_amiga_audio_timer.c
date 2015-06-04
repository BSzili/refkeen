//#include "SDL.h"
//#include <proto/timer.h>
//#include <dos/dos.h>
//#include <graphics/gfxbase.h>
//#include <dos/dostags.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
//#include <unistd.h>
//#include <sys/time.h>
#include "debug_amiga.h"

#include "be_cross.h"
#include "be_st.h"
//#include "opl/dbopl.h"

#if defined(__AMIGA__)
#define IPTR ULONG
#endif

#define PC_PIT_RATE 1193182

static bool g_sdlEmulatedOPLChipReady;
static void (*g_sdlCallbackSDFuncPtr)(void) = 0;

// A PRIVATE g_sdlTimeCount variable we store
// (SD_GetTimeCount/SD_SetTimeCount should be called instead)
static uint32_t g_sdlTimeCount;

static uint32_t g_timerDelay = 0;

// hack to add my own timer
extern void SD_SetUserHook(void (*hook)(void));

static void MySoundUserHook(void)
{
	g_sdlTimeCount++;
}

#if defined(__AMIGA__)
APTR g_timerIntHandle = NULL;

static void BEL_ST_TimerInterrupt(register APTR *data __asm("a1"))
{
	g_sdlCallbackSDFuncPtr();
}

static BOOL BEL_ST_AddTimerInt(void)
{
	if (g_timerIntHandle)
		return TRUE;

	if ((g_timerIntHandle = AddTimerInt((APTR)BEL_ST_TimerInterrupt, NULL)))
	{
		StartTimerInt(g_timerIntHandle, g_timerDelay, TRUE);
		return TRUE;
	}

	printf("Couldn't start the timer\n");

	return FALSE;
}

static void BEL_ST_RemTimerInt(void)
{
	RemTimerInt(g_timerIntHandle);
	g_timerIntHandle = NULL;
}

#else

BOOL g_runThread = FALSE;
struct Task *task;

static void timerThreadFunc(void)
{
	struct MsgPort *timerMsgPort;
	struct timerequest *timerIO;

	if ((timerMsgPort = CreateMsgPort()))
	{
		if ((timerIO = (struct timerequest *)CreateIORequest(timerMsgPort, sizeof(struct timerequest))))
		{
			if (!OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timerIO, 0))
			{
				timerIO->tr_node.io_Command = TR_ADDREQUEST;

				while (g_runThread)
				{
					g_sdlCallbackSDFuncPtr();

					timerIO->tr_time.tv_secs = 0;
					timerIO->tr_time.tv_micro = g_timerDelay;
					DoIO((struct IORequest *)timerIO);
				}

				CloseDevice((struct IORequest *)timerIO);
			}
			DeleteIORequest((struct IORequest *)timerIO);
		}
		DeleteMsgPort(timerMsgPort);
	}
}

static BOOL BEL_ST_AddTimerInt(void)
{
	struct TagItem processTags[] =
	{
		{NP_Entry, (IPTR)timerThreadFunc},
#ifdef __MORPHOS__
		{NP_CodeType, CODETYPE_PPC},
#endif
		{NP_Priority, 10},
		{TAG_DONE, 0}
	};

	if (g_runThread)
		return TRUE;

	g_runThread = TRUE;

	if ((task = (struct Task *)CreateNewProc(processTags)))
	{
		//StartTimer();
		return TRUE;
	}

	printf("Couldn't start the timer\n");

	return FALSE;
}

static void BEL_ST_RemTimerInt(void)
{
	g_runThread = FALSE;
	Delay(50);
	SD_SetUserHook(NULL);
}
#endif

void BE_ST_InitAudio(void)
{
	g_sdlTimeCount = 0;
	g_sdlEmulatedOPLChipReady = false;
}

void BE_ST_ShutdownAudio(void)
{
	g_sdlCallbackSDFuncPtr = 0; // Just in case this may be called after the audio subsystem was never really started (manual calls to callback)
}

void BE_ST_StartAudioSDService(void (*funcPtr)(void))
{
	D(bug("%s(%p)\n", __FUNCTION__, funcPtr));
	g_sdlCallbackSDFuncPtr = funcPtr;
	SD_SetUserHook(MySoundUserHook);
}

void BE_ST_StopAudioSDService(void)
{
	g_sdlCallbackSDFuncPtr = 0;
}

void BE_ST_LockAudioRecursively(void)
{
}

void BE_ST_UnlockAudioRecursively(void)
{
}

// Use this ONLY if audio subsystem isn't properly started up
void BE_ST_PrepareForManualAudioSDServiceCall(void)
{
}

bool BE_ST_IsEmulatedOPLChipReady(void)
{
	return g_sdlEmulatedOPLChipReady;
}

// Frequency is about 1193182Hz/spkVal
void BE_ST_PCSpeakerOn(uint16_t spkVal)
{
}

void BE_ST_PCSpeakerOff(void)
{
}

void BE_ST_BSound(uint16_t frequency)
{
}

void BE_ST_BNoSound(void)
{
}

// Drop-in replacement for id_sd.c:alOut
void BE_ST_ALOut(uint8_t reg,uint8_t val)
{
}

// Here, the actual rate is about 1193182Hz/speed
// NOTE: isALMusicOn is irrelevant for Keen Dreams (even with its music code)
void BE_ST_SetTimer(uint16_t speed, bool isALMusicOn)
{
	if (speed > 0)
	{
		g_timerDelay = (1000 * 1000) / (PC_PIT_RATE / speed);
		BEL_ST_AddTimerInt();
	}
	else
	{
		BEL_ST_RemTimerInt();
		g_timerDelay = 0;
	}
	D(bug("%s(%d,%d) delay %d\n", __FUNCTION__, speed, isALMusicOn, g_timerDelay));
}

void BEL_ST_UpdateHostDisplay(void);

uint32_t BE_ST_GetTimeCount(void)
{
	return g_sdlTimeCount;
}

void BE_ST_SetTimeCount(uint32_t newcount)
{
	Forbid();
	g_sdlTimeCount = newcount;
	Permit();
}

void BE_ST_TimeCountWaitForDest(uint32_t dsttimecount)
{
	BEL_ST_UpdateHostDisplay();
	while (g_sdlTimeCount<dsttimecount)
		BE_ST_ShortSleep();
}

void BE_ST_TimeCountWaitFromSrc(uint32_t srctimecount, int16_t timetowait)
{
	BEL_ST_UpdateHostDisplay();
	while (g_sdlTimeCount-srctimecount<timetowait)
		BE_ST_ShortSleep();
}

void BE_ST_WaitVBL(int16_t number)
{
	while (number-- > 0)
		WaitTOF();
}

// Call during a busy loop of some unknown duration (e.g., waiting for key press/release)
void BE_ST_ShortSleep(void)
{
	Delay(1);
	/*BYTE oldpri;
	struct Task *task;
	task = FindTask(NULL);
	oldpri = SetTaskPri(task, -10);
	SetTaskPri(task, oldpri);*/
}

void BE_ST_Delay(uint16_t msec) // Replacement for delay from dos.h
{
	Delay(msec/50);
}
