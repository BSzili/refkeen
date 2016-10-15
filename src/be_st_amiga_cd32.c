#include <proto/nonvolatile.h>
#define Point Point_C3D

#ifdef REFKEEN_VER_CAT3D
#include "id91_11/cat_all/cat3d/c3_def.h"
#define APPNAME "CAT3D"
#elif defined(REFKEEN_VER_CATABYSS)
#include "id91_11/cat_all/catabyss/def.h"
#define APPNAME "CATABYSS"
#elif defined(REFKEEN_VER_CATARM)
#include "id91_11/cat_all/catarm/def.h"
#define APPNAME "CATARM"
#elif defined(REFKEEN_VER_CATAPOC)
#include "id91_11/cat_all/catapoc/def.h"
#define APPNAME "CATAPOC"
#else
#error WAT?
#endif

#define SAVEITEMNAME "SAVE"
#define SCOREITEMNAME "SCORE"

typedef struct
{
#ifdef REFKEEN_VER_CAT3D
	unsigned int difficulty : 2; // gd_Continue, gd_Easy, gd_Normal, gd_Hard 0-3
#else
	unsigned int difficulty : 1; // EASYMODEON 0-1
#endif
	unsigned int mapon : 5;
	unsigned int bolts : 7,
				nukes : 7,
				potions : 7; // 0 - 99
	unsigned int key0 : 7,
				key1 : 7,
				key2 : 7,
				key3 : 7; // 0 - 99
#if defined(REFKEEN_VER_CAT3D) || defined(REFKEEN_VER_CATABYSS)
	unsigned int scroll0 : 1,
				scroll1 : 1,
				scroll2 : 1,
				scroll3 : 1,
				scroll4 : 1,
				scroll5 : 1,
				scroll6 : 1,
				scroll7 : 1;
#endif
#ifndef REFKEEN_VER_CAT3D
	unsigned int gem0 : 1,
				gem1 : 1,
				gem2 : 1,
				gem3 : 1,
				gem4 : 1; // 0 or GEM_DELAY_TIME
#else
	unsigned int score : 16; // score is always multiple of 100, and I doubt it will ever exceed 6553500
#endif
	unsigned int body : 7; // MAXBODY
} __attribute__((__packed__)) packedstate;

packedstate savedstate;

void BE_ST_CompressState(void)
{
#ifdef REFKEEN_VER_CAT3D
	savedstate.difficulty = gamestate.difficulty;
#else
	savedstate.difficulty = EASYMODEON;
#endif
	savedstate.mapon = gamestate.mapon;
	savedstate.bolts = gamestate.bolts;
	savedstate.nukes = gamestate.nukes;
	savedstate.potions = gamestate.potions;
	savedstate.key0 = gamestate.keys[0];
	savedstate.key1 = gamestate.keys[1];
	savedstate.key2 = gamestate.keys[2];
	savedstate.key3 = gamestate.keys[3];
#if defined(REFKEEN_VER_CAT3D) || defined(REFKEEN_VER_CATABYSS)
	savedstate.scroll0 = gamestate.scrolls[0];
	savedstate.scroll1 = gamestate.scrolls[1];
	savedstate.scroll2 = gamestate.scrolls[2];
	savedstate.scroll3 = gamestate.scrolls[3];
	savedstate.scroll4 = gamestate.scrolls[4];
	savedstate.scroll5 = gamestate.scrolls[5];
	savedstate.scroll6 = gamestate.scrolls[6];
	savedstate.scroll7 = gamestate.scrolls[7];
#endif
#ifndef REFKEEN_VER_CAT3D
	savedstate.gem0 = (gamestate.gems[0] == GEM_DELAY_TIME) ? true : false;
	savedstate.gem1 = (gamestate.gems[1] == GEM_DELAY_TIME) ? true : false;
	savedstate.gem2 = (gamestate.gems[2] == GEM_DELAY_TIME) ? true : false;
	savedstate.gem3 = (gamestate.gems[3] == GEM_DELAY_TIME) ? true : false;
	savedstate.gem4 = (gamestate.gems[4] == GEM_DELAY_TIME) ? true : false;
#else
	savedstate.score = gamestate.score / 100;
#endif
	savedstate.body = gamestate.body;
}

void BE_ST_DecompressState(void)
{
#ifdef REFKEEN_VER_CAT3D
	gamestate.difficulty = savedstate.difficulty;
#else
	EASYMODEON = savedstate.difficulty;
#endif
	gamestate.mapon = savedstate.mapon;
	gamestate.bolts = savedstate.bolts;
	gamestate.nukes = savedstate.nukes;
	gamestate.potions = savedstate.potions;
	gamestate.keys[0] = savedstate.key0;
	gamestate.keys[1] = savedstate.key1;
	gamestate.keys[2] = savedstate.key2;
	gamestate.keys[3] = savedstate.key3;
#if defined(REFKEEN_VER_CAT3D) || defined(REFKEEN_VER_CATABYSS)
	gamestate.scrolls[0] = savedstate.scroll0;
	gamestate.scrolls[1] = savedstate.scroll1;
	gamestate.scrolls[2] = savedstate.scroll2;
	gamestate.scrolls[3] = savedstate.scroll3;
	gamestate.scrolls[4] = savedstate.scroll4;
	gamestate.scrolls[5] = savedstate.scroll5;
	gamestate.scrolls[6] = savedstate.scroll6;
	gamestate.scrolls[7] = savedstate.scroll7;
#endif
#ifndef REFKEEN_VER_CAT3D
	gamestate.gems[0] = savedstate.gem0 ? GEM_DELAY_TIME : 0;
	gamestate.gems[1] = savedstate.gem1 ? GEM_DELAY_TIME : 0;
	gamestate.gems[2] = savedstate.gem2 ? GEM_DELAY_TIME : 0;
	gamestate.gems[3] = savedstate.gem3 ? GEM_DELAY_TIME : 0;
	gamestate.gems[4] = savedstate.gem4 ? GEM_DELAY_TIME : 0;
#else
	gamestate.score = savedstate.score * 100;
#endif
	gamestate.body = savedstate.body;
}

static bool BEL_ST_StoreNV(STRPTR name, APTR data, ULONG length)
{
	UWORD error;
	ULONG nvsize = (length + 9) / 10; // size in the units of tens of bytes

	error = StoreNV(APPNAME, name, data, nvsize, TRUE);

	return error == 0;
}

static bool BEL_ST_GetNV(STRPTR name, APTR data, ULONG length)
{
	APTR nvdata;

	if ((nvdata = GetCopyNV(APPNAME, name, TRUE)))
	{
		memcpy(data, nvdata, length);
		FreeNVData(nvdata);

		return true;
	}

	return false;
}

int BE_ST_SaveState(char *filename)
{
	//return BEL_ST_StoreNV(SAVEITEMNAME, &savedstate, sizeof(savedstate));
	if (BEL_ST_StoreNV(filename, &savedstate, sizeof(savedstate)))
	{
		return savedstate.mapon+1;
	}
	return 0;
}

bool BE_ST_RestoreState(char *filename)
{
	//return BEL_ST_GetNV(SAVEITEMNAME, &savedstate, sizeof(savedstate));
	// backward compatibility hack
	if (BEL_ST_GetNV(filename, &savedstate, sizeof(savedstate)))
		return true;
	if (!strcmp(filename, "SAVEGAM0.C3D"))
		filename = SAVEITEMNAME;

	return BEL_ST_GetNV(filename, &savedstate, sizeof(savedstate));
}

int BE_ST_SaveExists(char *filename)
{
	if (BE_ST_RestoreState(filename))
	{
		return savedstate.mapon+1;
	}
	return 0;
}

#if 0
bool BE_ST_SaveScores(void)
{
	return BEL_ST_StoreNV(SCOREITEMNAME, Scores, sizeof(HighScore) * MaxScores);
}

bool BE_ST_ReadScores(void)
{
	return BEL_ST_GetNV(SCOREITEMNAME, Scores, sizeof(HighScore) * MaxScores);
}
#endif

#include <proto/exec.h>
#include <devices/cd.h>

static struct IOStdReq *g_cdReq = NULL;
static struct MsgPort *g_cdPort = NULL;
static bool g_cdOpen = false;
static int g_lastCDTrack = 0;

void BE_ST_CloseCD(void)
{
	if (g_cdOpen)
	{
		CloseDevice((struct IORequest *)g_cdReq);
		g_cdOpen = false;
	}

	if (g_cdReq)
	{
		DeleteIORequest((struct IORequest *)g_cdReq);
		g_cdReq = NULL;
	}

	if (g_cdPort)
	{
		DeleteMsgPort(g_cdPort);
		g_cdPort = NULL;
	}
}

bool BE_ST_OpenCD(void)
{
	if (!(g_cdPort = CreateMsgPort()))
	{
		BE_ST_CloseCD();
		return false;
	}

	if (!(g_cdReq = CreateIORequest(g_cdPort, sizeof(*g_cdReq))))
	{
		BE_ST_CloseCD();
		return false;
	}

	if (OpenDevice("cd.device", 0, (struct IORequest *)g_cdReq, 0))
	{
		BE_ST_CloseCD();
		return false;
	}

	g_cdOpen = true;
	return true;
}

void BE_ST_PauseCD(bool pause)
{
	if (!g_cdReq)
		return;

	g_cdReq->io_Command = CD_PAUSE;
	g_cdReq->io_Length = pause ? 1 : 0;
	DoIO((struct IORequest *)g_cdReq);
}

void BE_ST_PlayCD(int track)
{
	if (!g_cdReq)
		return;

	g_cdReq->io_Command = CD_PLAYTRACK;
	g_cdReq->io_Length = 1;
	g_cdReq->io_Offset = track;
	SendIO((struct IORequest *)g_cdReq);
	g_lastCDTrack = track;
}

void BE_ST_StopCD(void)
{
	if (!g_cdReq)
		return;

	if (!CheckIO((struct IORequest *)g_cdReq))
	{
		AbortIO((struct IORequest *)g_cdReq);
		WaitIO((struct IORequest *)g_cdReq);
	}
	g_lastCDTrack = 0;
}

void BE_ST_SetCDVolume(int vol)
{
	g_cdReq->io_Command = CD_ATTENUATE;
	g_cdReq->io_Length = 1;
	g_cdReq->io_Offset = vol;
	DoIO((struct IORequest *)g_cdReq);
}

void BE_ST_StartMusic(void)
{
	if (BE_ST_OpenCD())
	{
		// first track is the data
		BE_ST_PlayCD(2);
	}
}

void BE_ST_StopMusic(void)
{
	BE_ST_StopCD();
	BE_ST_CloseCD();
}

void BE_ST_UpdateMusic(void)
{
	if (g_lastCDTrack > 0 && CheckIO((struct IORequest *)g_cdReq))
	{
		BE_ST_PlayCD(g_lastCDTrack);
	}
}
