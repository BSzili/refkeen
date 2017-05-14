//#include "SDL.h"
//#include <proto/timer.h>
//#include <dos/dos.h>
#include <graphics/gfxbase.h>
//#include <dos/dostags.h>
#include <devices/audio.h>
#include <hardware/cia.h>
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
#define LASTSOUND 37 // Catacomb Apocalypse
#define OPL_SAMPLE_RATE 11025 // converted sound sample rate

#define TIMER_SIGNAL
//#define NO_FILTER
#define USE_DIGI_SOUND

static bool g_sdlEmulatedOPLChipReady;
static void (*g_sdlCallbackSDFuncPtr)(void) = 0;

// A PRIVATE g_sdlTimeCount variable we store
// (SD_GetTimeCount/SD_SetTimeCount should be called instead)
static uint32_t g_sdlTimeCount;
static uint32_t g_dstTimeCount;
static struct Task *g_mainTask;

static uint32_t g_timerDelay = 0;

static struct MsgPort *g_audioPort;
static struct IOAudio *g_audioReq;
static BYTE *g_squareWave;
static LONG g_audioClock;
static APTR g_timerIntHandle = NULL;
#ifdef NO_FILTER
static struct CIA *ciaa = (struct CIA *)0xbfe001;
static bool g_restoreFilter;
#endif
#ifdef USE_DIGI_SOUND
static int8_t *g_digiSounds[LASTSOUND];
static int g_digiSoundSamples[LASTSOUND];
static int g_lastSound = -1;
#endif

// hack to add my own timer
extern void SD_SetUserHook(void (*hook)(void));

static void MySoundUserHook(void)
{
	g_sdlTimeCount++;
#ifdef TIMER_SIGNAL
	Forbid();
	if (g_sdlTimeCount >= g_dstTimeCount)
	{
		Signal(g_mainTask, SIGBREAKF_CTRL_F);
		g_dstTimeCount = (uint32_t)~1;
	}
	Permit();
#endif
}

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
		g_dstTimeCount = (uint32_t)~1;
		g_mainTask = FindTask(NULL);
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

#ifdef USE_DIGI_SOUND
static void BEL_ST_LoadDigiSounds(void)
{
	char filename[256];
	BE_FILE_T fp;

	for (int i = 0; i < LASTSOUND; i++)
	{
		snprintf(filename, sizeof(filename), "asound%02d.raw", i);
		if ((fp = BE_Cross_open_readonly_for_reading(filename)))
		//if ((fp = fopen(filename, "rb")))
		{
			int32_t nbyte = BE_Cross_FileLengthFromHandle(fp);
			if ((g_digiSounds[i] = AllocVec(nbyte, MEMF_CHIP|MEMF_PUBLIC)))
			{
				BE_Cross_readInt8LEBuffer(fp, g_digiSounds[i], nbyte);
				g_digiSoundSamples[i] = nbyte;
				g_sdlEmulatedOPLChipReady = true;
			}
			BE_Cross_close(fp);
		}
	}
}

static void BEL_ST_FreeDigiSounds(void)
{
	for (int i = 0; i < LASTSOUND; i++)
	{
		FreeVec(g_digiSounds[i]);
		g_digiSounds[i] = NULL;
		g_digiSoundSamples[i] = 0;
	}
}
#endif

void BE_ST_ShutdownAudio(void);

void BE_ST_InitAudio(void)
{
	//UBYTE whichannel[] = {1, 2, 4, 8};
	UBYTE whichannel[] = {3, 5, 10, 12};

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
#ifdef USE_DIGI_SOUND
					BEL_ST_LoadDigiSounds();
#endif
#ifdef NO_FILTER
					// turn off the 3.2 kHz low-pass filter
					g_restoreFilter = !(ciaa->ciapra & CIAF_LED) ? true : false;
					ciaa->ciapra |= CIAF_LED;
#endif
					return;
				}
			}
		}
	}
	BE_ST_ShutdownAudio();
}

void BE_ST_ShutdownAudio(void)
{
#ifdef USE_DIGI_SOUND
	BEL_ST_FreeDigiSounds();
#endif

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

#ifdef NO_FILTER
	if (g_restoreFilter)
	{
		// restore filter
		ciaa->ciapra &= ~CIAF_LED;
	}
#endif
}

void BE_ST_StartAudioAndTimerInt(void (*funcPtr)(void))
{
	D(bug("%s(%p)\n", __FUNCTION__, funcPtr));
	g_sdlCallbackSDFuncPtr = funcPtr;
	SD_SetUserHook(MySoundUserHook);
}

void BE_ST_StopAudioAndTimerInt(void)
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

static void BEL_ST_PlaySample(int8_t *sample, int length, int cycles, int rate)
{
	g_audioReq->ioa_Request.io_Command = CMD_FLUSH;
	DoIO((struct IORequest *)g_audioReq);

	g_audioReq->ioa_Request.io_Command = CMD_WRITE;
	g_audioReq->ioa_Request.io_Flags = ADIOF_PERVOL;
	g_audioReq->ioa_Data = (UBYTE *)sample;
	g_audioReq->ioa_Length = length;
	g_audioReq->ioa_Period = g_audioClock / rate;
	g_audioReq->ioa_Volume = 64;
	g_audioReq->ioa_Cycles = cycles;
	BeginIO((struct IORequest *)g_audioReq);
}

static bool BEL_ST_SamplePlaying(void)
{
	return !CheckIO((struct IORequest *)g_audioReq);
}

static void BEL_ST_StopSample(void)
{
	g_audioReq->ioa_Request.io_Command = CMD_STOP;
	BeginIO((struct IORequest *)g_audioReq);
	g_audioReq->ioa_Request.io_Command = CMD_RESET;
	BeginIO((struct IORequest *)g_audioReq);
}

bool BE_ST_SoundPlaying(void)
{
	return BEL_ST_SamplePlaying() && g_lastSound >= 0;
}

void BE_ST_PlaySound(int sound)
{
	if (!g_digiSounds[sound])
		return;

#ifdef REFKEEN_VER_CATACOMB_ALL
	// HACK don't let the walk sounds trample over the rest
	if (BE_ST_SoundPlaying() && (sound == 14 || sound == 15))
		return;
#endif

	g_lastSound = sound;
	BEL_ST_PlaySample(g_digiSounds[sound], g_digiSoundSamples[sound], 1, OPL_SAMPLE_RATE);
}

void BE_ST_StopSound(void)
{
	//g_lastSound = -1;
	BEL_ST_StopSample();
}

// Frequency is about 1193182Hz/spkVal
void BE_ST_PCSpeakerOn(uint16_t spkVal)
{
	if (!g_squareWave)
		return;

	/*
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
	*/
	BEL_ST_PlaySample(g_squareWave, SQUARE_SAMPLES, 0, SQUARE_SAMPLES * (PC_PIT_RATE / spkVal));
}

void BE_ST_PCSpeakerOff(void)
{
	if (!g_squareWave)
		return;

	/*
	g_audioReq->ioa_Request.io_Command = CMD_STOP;
	BeginIO((struct IORequest *)g_audioReq);
	g_audioReq->ioa_Request.io_Command = CMD_RESET;
	BeginIO((struct IORequest *)g_audioReq);
	*/
	BEL_ST_StopSample();
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
void BE_ST_OPL2Write(uint8_t reg,uint8_t val)
{
}

// Here, the actual rate is about 1193182Hz/speed
// NOTE: isALMusicOn is irrelevant for Keen Dreams (even with its music code)
void BE_ST_SetTimer(uint16_t speed)
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

uint32_t BE_ST_GetTimeCount(void)
{
	return g_sdlTimeCount;
}

/*
void BE_ST_SetTimeCount(uint32_t newcount)
{
	Forbid();
	g_sdlTimeCount = newcount;
	Permit();
}

void BE_ST_TimeCountWaitForDest(uint32_t dsttimecount)
{
#ifdef TIMER_SIGNAL
	if (g_sdlTimeCount >= dsttimecount)
		return;

	SetSignal(0, SIGBREAKF_CTRL_F);
	g_dstTimeCount = dsttimecount;
	Wait(SIGBREAKF_CTRL_F);
#else
	while (g_sdlTimeCount<dsttimecount)
		BE_ST_ShortSleep();
#endif
}

void BE_ST_TimeCountWaitFromSrc(uint32_t srctimecount, int16_t timetowait)
{
#ifdef TIMER_SIGNAL
	if (g_sdlTimeCount >= srctimecount+timetowait)
		return;
//printf("BE_ST_TimeCountWaitFromSrc %u %d %u\n", srctimecount, timetowait, g_sdlTimeCount);
	SetSignal(0, SIGBREAKF_CTRL_F);
	g_dstTimeCount = srctimecount+timetowait;
	Wait(SIGBREAKF_CTRL_F);
#else
	while (g_sdlTimeCount-srctimecount<timetowait)
		BE_ST_ShortSleep();
#endif
}
*/

void BE_ST_WaitForNewVerticalRetraces(int16_t number)
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
	//TimeDelay(UNIT_MICROHZ, 0, 500);
	BE_ST_PollEvents();
}

void BE_ST_Delay(uint16_t msec) // Replacement for delay from dos.h
{
	Delay(msec/2);
}

// Resets to 0 an internal counter of calls to timer interrupt,
// and returns the original counter's value
int BE_ST_TimerIntClearLastCalls(void)
{
	uint32_t count = g_sdlTimeCount;
	Forbid();
	g_sdlTimeCount = 0;
	Permit();
	return count;
}

// Attempts to wait for a given amount of calls to timer interrupt.
// It may wait a bit more in practice (e.g., due to Sync to VBlank).
// This is taken into account into a following call to the same function,
// which may actually be a bit shorter than requested (as a consequence).
void BE_ST_TimerIntCallsDelayWithOffset(int nCalls)
{
	uint32_t dsttimecount = g_sdlTimeCount+nCalls;
#ifdef TIMER_SIGNAL
	if (g_sdlTimeCount >= dsttimecount)
		return;

	SetSignal(0, SIGBREAKF_CTRL_F);
	g_dstTimeCount = dsttimecount;
	Wait(SIGBREAKF_CTRL_F);
#else
	while (g_sdlTimeCount<dsttimecount)
		BE_ST_ShortSleep();
#endif
}

// Keen Dreams 2015 digitized sounds playback

void BE_ST_PlayS16SoundEffect(int16_t *data, int numOfSamples)
{
}

void BE_ST_StopSoundEffect(void)
{
}
