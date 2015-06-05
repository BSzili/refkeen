//#include "SDL.h"
//#include <proto/timer.h>
//#include <dos/dos.h>
#include <graphics/gfxbase.h>
//#include <dos/dostags.h>
#include <devices/audio.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/lowlevel.h>
#include <clib/alib_protos.h>
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
#define SQUARE_SAMPLES 2

static bool g_sdlEmulatedOPLChipReady;
static void (*g_sdlCallbackSDFuncPtr)(void) = 0;

// A PRIVATE g_sdlTimeCount variable we store
// (SD_GetTimeCount/SD_SetTimeCount should be called instead)
static uint32_t g_sdlTimeCount;

static uint32_t g_timerDelay = 0;

static struct MsgPort *g_audioPort;
static struct IOAudio *g_audioReq;
static BYTE *g_squareWave;
static LONG g_audioClock;

// hack to add my own timer
extern void SD_SetUserHook(void (*hook)(void));

static void MySoundUserHook(void)
{
	g_sdlTimeCount++;
}


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

void BE_ST_ShutdownAudio(void);

void BE_ST_InitAudio(void)
{
	UBYTE whichannel[] = {1, 2, 4, 8};

	g_sdlTimeCount = 0;
	g_sdlEmulatedOPLChipReady = false;

	if (GfxBase->DisplayFlags & PAL)
		g_audioClock = 3546895;
	else
		g_audioClock = 3579545;

	if ((g_audioPort = CreateMsgPort()))
	{
		if ((g_audioReq = (struct IOAudio *)CreateIORequest(g_audioPort, sizeof(struct IOAudio))))
		{
			g_audioReq->ioa_Request.io_Command = ADCMD_ALLOCATE;
			g_audioReq->ioa_Request.io_Flags = ADIOF_NOWAIT;
			g_audioReq->ioa_AllocKey = 0;
			g_audioReq->ioa_Data = whichannel;
			g_audioReq->ioa_Length = sizeof(whichannel);
			if (!OpenDevice((STRPTR)"audio.device", 0, (struct IORequest *)g_audioReq, 0))
			{
				if ((g_squareWave = (BYTE *)AllocVec(SQUARE_SAMPLES, MEMF_CHIP|MEMF_PUBLIC)))
				{
					g_squareWave[0] = 127;
					g_squareWave[1] = -127;
					return;
				}
			}
		}
	}
	BE_ST_ShutdownAudio();
}

void BE_ST_ShutdownAudio(void)
{
	if (g_audioReq)
	{
		// Should I free the channel here?
		/*g_audioReq->ioa_Request.io_Command = ADCMD_FREE;
		DoIO((struct IORequest *)g_audioReq);*/
		/*AbortIO((struct IORequest *)g_audioReq);
		WaitIO((struct IORequest *)g_audioReq);*/
		CloseDevice((struct IORequest *)g_audioReq);
		DeleteIORequest((struct IORequest *)g_audioReq);
		g_audioReq = NULL;
	}

	if (g_audioPort)
	{
		DeleteMsgPort(g_audioPort);
		g_audioPort = NULL;
	}

	if (g_squareWave)
	{
		FreeVec(g_squareWave);
		g_squareWave = NULL;
	}
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
	if (!g_squareWave)
		return;

	g_audioReq->ioa_Request.io_Command = CMD_FLUSH;
	DoIO((struct IORequest *)g_audioReq);
	//AbortIO((struct IORequest *)g_audioReq);

	g_audioReq->ioa_Request.io_Command = CMD_WRITE;
	g_audioReq->ioa_Request.io_Flags = ADIOF_PERVOL;
	g_audioReq->ioa_Data = (UBYTE *)g_squareWave;
	g_audioReq->ioa_Length = SQUARE_SAMPLES;
	g_audioReq->ioa_Period = g_audioClock / (SQUARE_SAMPLES * (PC_PIT_RATE / spkVal));
	g_audioReq->ioa_Volume = 64;
	g_audioReq->ioa_Cycles = 0;
	BeginIO((struct IORequest *)g_audioReq);
}

void BE_ST_PCSpeakerOff(void)
{
	if (!g_squareWave)
		return;

	g_audioReq->ioa_Request.io_Command = CMD_STOP;
	BeginIO((struct IORequest *)g_audioReq);
	g_audioReq->ioa_Request.io_Command = CMD_RESET;
	BeginIO((struct IORequest *)g_audioReq);
}

// only used in FindFile()
void BE_ST_BSound(uint16_t frequency)
{
	BE_ST_PCSpeakerOn(PC_PIT_RATE / frequency);
}

void BE_ST_BNoSound(void)
{
	BE_ST_PCSpeakerOff();
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
	//BEL_ST_UpdateHostDisplay();
	while (g_sdlTimeCount<dsttimecount)
		BE_ST_ShortSleep();
}

void BE_ST_TimeCountWaitFromSrc(uint32_t srctimecount, int16_t timetowait)
{
	//BEL_ST_UpdateHostDisplay();
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
	Delay(msec/2);
}
