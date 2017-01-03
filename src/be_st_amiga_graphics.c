#include <string.h>
//#include "SDL.h"
#include <graphics/gfxmacros.h>
#include <graphics/videocontrol.h>
#include <exec/execbase.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include "debug_amiga.h"

#include "be_cross.h"
#include "be_st.h"

// for the PAGE?START macros
#ifdef REFKEEN_VER_CAT3D
#ifdef KALMS_C2P
#define VIEWWIDTH (32*8)
#else
#define VIEWWIDTH (33*8)
#endif
#define VIEWHEIGHT	(18*8)
#else
#define VIEWWIDTH (40*8)
#define VIEWHEIGHT	(15*8)
#endif
#define VIEWX		0
#define VIEWY		0 
#define VIEWXH		(VIEWX+VIEWWIDTH-1)
#define VIEWYH		(VIEWY+VIEWHEIGHT-1)

#ifdef REFKEEN_VER_KDREAMS
#define	SCREENWIDTH_EGA		64
#endif

#define GFX_TEX_WIDTH 320
#define GFX_TEX_HEIGHT 200

#define TXT_COLS_NUM 80
#define TXT_ROWS_NUM 25

#if defined(__AMIGA__)
#define IPTR ULONG
#endif

#define ENABLE_TEXT
#define TEXT_CGA_FONT
//#define USE_SPLITSCREEN

extern struct Custom custom;


// We can use a union because the memory contents are refreshed on screen mode change
// (well, not on change between modes 0xD and 0xE, both sharing planar A000:0000)
union bufferType {
	uint8_t egaGfx[4][0x10000]; // Contents of A000:0000 (4 planes)
	//uint8_t text[TXT_COLS_NUM*TXT_ROWS_NUM*2]; // Textual contents of B800:0000
};

// EGA 16-color palette in LoadRGB32 friendly format
static const uint32_t g_amigaEGAPalette[] = {
	0x00000000, 0x00000000, 0x00000000, /*black*/
	0x00000000, 0x00000000, 0xaa000000, /*blue*/
	0x00000000, 0xaa000000, 0x00000000, /*green*/
	0x00000000, 0xaa000000, 0xaa000000, /*cyan*/
	0xaa000000, 0x00000000, 0x00000000, /*red*/
	0xaa000000, 0x00000000, 0xaa000000, /*magenta*/
	0xaa000000, 0x55000000, 0x00000000, /*brown*/
	0xaa000000, 0xaa000000, 0xaa000000, /*light gray*/
	0x55000000, 0x55000000, 0x55000000, /*gray*/
	0x55000000, 0x55000000, 0xff000000, /*light blue*/
	0x55000000, 0xff000000, 0x55000000, /*light green*/
	0x55000000, 0xff000000, 0xff000000, /*light cyan*/
	0xff000000, 0x55000000, 0x55000000, /*light red*/
	0xff000000, 0x55000000, 0xff000000, /*light magenta*/
	0xff000000, 0xff000000, 0x55000000, /*light yellow*/
	0xff000000, 0xff000000, 0xff000000, /*white*/
};

struct Screen *g_amigaScreen = NULL;
struct Window *g_amigaWindow = NULL;
#ifdef USE_SPLITSCREEN
static struct Screen *g_splitScreen = NULL;
#endif

static union bufferType *g_sdlVidMem;
static uint8_t g_textMemory[TXT_COLS_NUM*TXT_ROWS_NUM*2];
#ifdef ENABLE_TEXT
static uint8_t g_textMemoryOld[TXT_COLS_NUM*TXT_ROWS_NUM*2];
#endif
static uint16_t g_sdlScreenStartAddress = 0;
static int g_sdlScreenMode = -1;
static int g_sdlTexWidth, g_sdlTexHeight;
static uint8_t g_sdlPelPanning = 0;
static int g_sdlLineWidth = 40;
static int16_t g_sdlSplitScreenLine = -1;

static struct BitMap g_screenBitMaps[/*4*/2];
static int g_currentBitMap;
static struct DBufInfo *dbuf = NULL;
static struct UCopList *ucl = NULL;
#ifdef ENABLE_TEXT
static UWORD *g_vgaFont = NULL;
static int g_sdlTxtCursorPosX, g_sdlTxtCursorPosY;
static bool g_sdlTxtCursorEnabled = true;
static int g_sdlTxtColor = 7, g_sdlTxtBackground = 0;
//extern const uint8_t g_vga_8x16TextFont[256*8*16];
#include "be_textmode_amiga.h"
#ifdef TEXT_CGA_FONT
#define FONT_HEIGHT 8
#define FONT_FACE g_cgaPackedFont
#else
#define FONT_HEIGHT 16
#define FONT_FACE g_vgaPackedFont
#endif
#endif
static UWORD *g_pointerMem = NULL;
/*static*/unsigned char *g_chunkyBuffer = NULL;

void BEL_ST_UpdateHostDisplay(void);
void BE_ST_MarkGfxForUpdate(void)
{
#ifdef ENABLE_TEXT
	BEL_ST_UpdateHostDisplay();
#endif
}

/* Gets a value represeting 6 EGA signals determining a color number and
 * returns it in a "Blue Green Red Intensity" 4-bit format.
 * Usually, the 6 signals represented by the given input mean:
 * "Blue Green Red Secondary-Blue Secondary-Green Secondary-Red". However, for
 * the historical reason of compatibility with CGA monitors, on the 200-lines
 * modes used by Keen the Secondary-Green signal is treated as an Intensity
 * one and the two other intensity signals are ignored.
 */
static int BEL_ST_ConvertEGASignalToEGAEntry(int color)
{
	return (color & 7) | ((color & 16) >> 1);
}

static void BEL_ST_SetPalette(int colors, int firstcolor, const uint8_t *palette)
{
	ULONG palette32[16 * 3 + 2];
	int paletteColor;
	int entry;

	D(bug("%s(%d, %d, %p)\n", __FUNCTION__, colors, firstcolor, palette));

	palette32[0] = colors << 16 | firstcolor;
	for (entry = 0; entry < colors; ++entry)
	{
		if (palette)
			paletteColor = BEL_ST_ConvertEGASignalToEGAEntry(palette[entry]) * 3;
		else
			paletteColor = entry * 3;
		palette32[entry * 3 + 1] = g_amigaEGAPalette[paletteColor];
		palette32[entry * 3 + 2] = g_amigaEGAPalette[paletteColor + 1];
		palette32[entry * 3 + 3] = g_amigaEGAPalette[paletteColor + 2];
	}
	palette32[entry * 3 + 1] = 0;
	LoadRGB32(&g_amigaScreen->ViewPort, palette32);
#ifdef USE_SPLITSCREEN
	if (g_splitScreen)
		LoadRGB32(&g_splitScreen->ViewPort, palette32);
#endif
}

static void BEL_ST_PrepareBitmap(struct BitMap *BM, uint16_t firstByte)
{
	InitBitMap(BM, 4, 8*g_sdlLineWidth, g_sdlTexHeight);

	BM->Planes[0] = &g_sdlVidMem->egaGfx[0][firstByte];
	BM->Planes[1] = &g_sdlVidMem->egaGfx[1][firstByte];
	BM->Planes[2] = &g_sdlVidMem->egaGfx[2][firstByte];
	BM->Planes[3] = &g_sdlVidMem->egaGfx[3][firstByte];
}

static void BEL_ST_CloseScreen(void)
{
	D(bug("%s()\n", __FUNCTION__));

	if (g_amigaWindow)
	{
		CloseWindow(g_amigaWindow);
		g_amigaWindow = NULL;
	}
	
	if (g_amigaScreen)
	{
		CloseScreen(g_amigaScreen);
		g_amigaScreen = NULL;
	}

	if (dbuf)
	{
#if 0
		DeleteMsgPort(dbuf->dbi_SafeMessage.mn_ReplyPort);
		DeleteMsgPort(dbuf->dbi_DispMessage.mn_ReplyPort);
#endif
		FreeDBufInfo(dbuf);
		dbuf = NULL;
	}
}

static void BEL_ST_ReopenScreen(void)
{
	ULONG modeid = BestModeID(
		BIDTAG_NominalWidth, g_sdlTexWidth,
		BIDTAG_NominalHeight, g_sdlTexHeight,
		BIDTAG_Depth, 4,
		BIDTAG_MonitorID, DEFAULT_MONITOR_ID,
		TAG_DONE);

	static struct Rectangle rect;
	rect.MinX = 0;
	rect.MaxX = g_sdlTexWidth - 1;
	rect.MinY = 0;
	rect.MaxY = g_sdlTexHeight - 1;

	D(bug("%s()\n", __FUNCTION__));

	BEL_ST_CloseScreen();

	// shut down the CD32 startup animation
	CloseLibrary(OpenLibrary("freeanim.library", 0)); 

	g_currentBitMap = 0;
	BEL_ST_PrepareBitmap(&g_screenBitMaps[g_currentBitMap], 0);

	if ((g_amigaScreen = OpenScreenTags(NULL, 
		SA_DisplayID, modeid,
		SA_AutoScroll, FALSE,
		(8*g_sdlLineWidth == g_sdlTexWidth) ? TAG_IGNORE : SA_DClip, (ULONG)&rect,
		SA_Width, 8*g_sdlLineWidth,
		SA_Height, g_sdlTexHeight,
		SA_Depth, 4,
		SA_ShowTitle, FALSE,
		SA_Quiet, TRUE,
		SA_Draggable, FALSE,
		SA_BitMap, (IPTR)&g_screenBitMaps[g_currentBitMap],
		SA_Type, CUSTOMSCREEN,
		TAG_DONE)))
	{
		if ((dbuf = AllocDBufInfo(&g_amigaScreen->ViewPort)))
		{
#if 0
			dbuf->dbi_SafeMessage.mn_ReplyPort = CreateMsgPort();
			dbuf->dbi_DispMessage.mn_ReplyPort = CreateMsgPort();
#endif

			D(bug("Screen ModeID: %08lx\n", GetVPModeID(&g_amigaScreen->ViewPort)));

			// faster ChangeVPBitMap / ScrollVPort
			VideoControlTags(g_amigaScreen->ViewPort.ColorMap,
				VC_IntermediateCLUpdate, FALSE,
				TAG_DONE); 

			BEL_ST_SetPalette(16, 0, NULL);

			if ((g_amigaWindow = OpenWindowTags(NULL,
				WA_Flags, WFLG_BACKDROP | WFLG_REPORTMOUSE | WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_RMBTRAP,
				WA_InnerWidth, g_sdlTexWidth,
				WA_InnerHeight, g_sdlTexHeight,
				WA_CustomScreen, (IPTR)g_amigaScreen,
				TAG_DONE)))
			{
				SetPointer(g_amigaWindow, g_pointerMem, 16, 16, 0, 0);
				return;
			}
		}
	}

	BE_ST_QuickExit();
}

void BE_ST_InitGfx(void)
{
	g_sdlVidMem = AllocVec(sizeof(*g_sdlVidMem), MEMF_CHIP | MEMF_CLEAR);
#ifdef ENABLE_TEXT
	g_vgaFont = AllocVec(sizeof(UWORD)*FONT_HEIGHT*256, MEMF_CHIP | MEMF_CLEAR);

	for (int currChar = 0; currChar < 256*FONT_HEIGHT; currChar++)
	{
		g_vgaFont[currChar] = FONT_FACE[currChar] << 8;
	}
#endif
	g_pointerMem = (UWORD *)AllocVec(16 * 16, MEMF_CLEAR | MEMF_CHIP);
#ifdef REFKEEN_VER_CATACOMB_ALL
	g_chunkyBuffer = (UBYTE *)AllocVec(VIEWWIDTH * VIEWHEIGHT, MEMF_ANY | MEMF_CLEAR);
#endif

	BE_ST_SetScreenMode(3);
}

void BE_ST_ShutdownGfx(void)
{
	BEL_ST_CloseScreen();
	FreeVec(g_sdlVidMem);
#ifdef ENABLE_TEXT
	FreeVec(g_vgaFont);
#endif
	FreeVec(g_pointerMem);
#ifdef REFKEEN_VER_CATACOMB_ALL
	FreeVec(g_chunkyBuffer);
#endif
}

void BE_ST_DebugText(int x, int y, const char *fmt, ...)
{
	UBYTE buffer[256];
	struct IntuiText WinText = {BLOCKPEN, DETAILPEN, JAM2, 0, 0, NULL, NULL, NULL};
	va_list ap;
	struct RastPort rp;

	WinText.IText = buffer;

	va_start(ap, fmt); 
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	//PrintIText(&g_amigaScreen->RastPort, &WinText, x, y);
	InitRastPort(&rp);
#ifdef REFKEEN_VER_CATACOMB_ALL
	struct BitMap bm;
	BEL_ST_PrepareBitmap(&bm, 0);
	rp.BitMap = &bm;
#else
	rp.BitMap = g_amigaScreen->ViewPort.RasInfo->BitMap;
#endif
	PrintIText(&rp, &WinText, x, y);
}

void BE_ST_DebugColor(uint16_t color)
{
#if 0
	UWORD colors[16] = {0x0000,0x000A,0x00A0,0x00AA,0x0A00,0x0A0A,0x0A50,0x0AAA,
						0x0555,0x055F,0x05F5,0x05FF,0x0F55,0x0F5F,0x0FF5,0x0FFF};
	custom.color[0] = colors[color%16];
#endif
}

#ifdef REFKEEN_VER_CATACOMB_ALL
// this doesn't really belong here
void BE_ST_ClearCache(void *address, uint32_t length)
{
	if ((SysBase->AttnFlags & AFF_68040) && !(address == NULL && length == ~0))
	{
		CacheClearE(address, length, CACRF_ClearI | CACRF_ClearD);
	}
	else
	{
		// according to the docs, this is faster on the 030 and below
		CacheClearU();
	}
}

#ifdef KALMS_C2P
extern void c2p1x1_4_c5_bm(
	register WORD chunkyx __asm("d0"),
	register WORD chunkyy __asm("d1"),
	register WORD offsx __asm("d2"),
	register WORD offsy __asm("d3"),
	register APTR chunkyscreen __asm("a0"),
	register struct BitMap *bitmap __asm("a1"));
#endif

void BE_ST_DrawChunkyBuffer(uint16_t bufferofs)
{
	struct BitMap bm;

	D(bug("%s(%u)\n", __FUNCTION__, bufferofs));

	// Akiko C2P for CD32
	if (GfxBase->LibNode.lib_Version >= 40 && GfxBase->ChunkyToPlanarPtr)
	{
		ULONG *c2p = GfxBase->ChunkyToPlanarPtr;
		ULONG *src = (ULONG *)g_chunkyBuffer;
		ULONG *dst0 = (ULONG *)&g_sdlVidMem->egaGfx[0][bufferofs];
		ULONG *dst1 = (ULONG *)&g_sdlVidMem->egaGfx[1][bufferofs];
		ULONG *dst2 = (ULONG *)&g_sdlVidMem->egaGfx[2][bufferofs];
		ULONG *dst3 = (ULONG *)&g_sdlVidMem->egaGfx[3][bufferofs];

		OwnBlitter();
		for (int i = 0; i < VIEWHEIGHT; i++)
		{
			for (int j = 0; j < VIEWWIDTH/(8*sizeof(ULONG)); j++)
			{
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*(GfxBase->ChunkyToPlanarPtr) = *src++;
				*dst0++ = *c2p;
				*dst1++ = *c2p;
				*dst2++ = *c2p;
				*dst3++ = *c2p;
			}
#if GFX_TEX_WIDTH > VIEWWIDTH
			dst0 += (GFX_TEX_WIDTH-VIEWWIDTH)/(8*sizeof(ULONG));
			dst1 += (GFX_TEX_WIDTH-VIEWWIDTH)/(8*sizeof(ULONG));
			dst2 += (GFX_TEX_WIDTH-VIEWWIDTH)/(8*sizeof(ULONG));
			dst3 += (GFX_TEX_WIDTH-VIEWWIDTH)/(8*sizeof(ULONG));
#endif
		}
		DisownBlitter();
		return;
	}

	BEL_ST_PrepareBitmap(&bm, bufferofs);
#ifdef KALMS_C2P
	c2p1x1_4_c5_bm(VIEWWIDTH, VIEWHEIGHT, VIEWX, VIEWY, g_chunkyBuffer, &bm);
#else
	struct RastPort rp;
	InitRastPort(&rp);
	rp.BitMap = &bm;
	WriteChunkyPixels(&rp, VIEWX, VIEWY, VIEWXH, VIEWYH, g_chunkyBuffer, VIEWWIDTH);
#endif
}
#endif

void BE_ST_SetScreenStartAddress(uint16_t crtc)
{
	D(bug("%s(%u)\n", __FUNCTION__, crtc));

	if (g_sdlScreenStartAddress != crtc)
	{
		g_sdlScreenStartAddress = crtc;
		g_currentBitMap ^= 1;
		BEL_ST_PrepareBitmap(&g_screenBitMaps[g_currentBitMap], g_sdlScreenStartAddress);
		ChangeVPBitMap(&g_amigaScreen->ViewPort, &g_screenBitMaps[g_currentBitMap], dbuf);
		WaitTOF();
		//WaitBOVP(&g_amigaScreen->ViewPort);
	}
}

// combined function for smooth(?) scrolling
void BE_ST_SetScreenStartAddressAndPelPanning(uint16_t crtc, uint8_t panning)
{
	D(bug("%s(%u,%u)\n", __FUNCTION__, crtc, panning));

#if 1
	uint16_t newScreenStartAddress = (crtc & (uint16_t)~1);
	uint8_t newPelPanning = panning + (crtc % 2)*8;

	if (g_sdlScreenStartAddress != newScreenStartAddress || g_sdlPelPanning != newPelPanning)
	{
		g_sdlScreenStartAddress = newScreenStartAddress;
		g_currentBitMap ^= 1;
		BEL_ST_PrepareBitmap(&g_screenBitMaps[g_currentBitMap], newScreenStartAddress);
		//ChangeVPBitMap(&g_amigaScreen->ViewPort, &g_screenBitMaps[g_currentBitMap], dbuf);
		g_amigaScreen->ViewPort.RasInfo->BitMap = &g_screenBitMaps[g_currentBitMap];
		g_sdlPelPanning = newPelPanning;
		g_amigaScreen->ViewPort.RasInfo->RxOffset = newPelPanning;
		ScrollVPort(&g_amigaScreen->ViewPort);
		WaitTOF();
		//WaitBOVP(&g_amigaScreen->ViewPort);
	}
#else
	if (g_sdlScreenStartAddress != crtc || g_sdlPelPanning != panning)
	{
		g_sdlScreenStartAddress = crtc;
		g_sdlPelPanning = panning;
		WaitTOF();
		g_currentBitMap ^= 1;
		BEL_ST_PrepareBitmap(&g_screenBitMaps[g_currentBitMap], (g_sdlScreenStartAddress & (uint16_t)~1));
		g_amigaScreen->ViewPort.RasInfo->RxOffset = g_sdlPelPanning + (g_sdlScreenStartAddress % 2)*8;
		ChangeVPBitMap(&g_amigaScreen->ViewPort, &g_screenBitMaps[g_currentBitMap], dbuf);
		ScrollVPort(&g_amigaScreen->ViewPort);
	}
#endif
}

uint8_t *BE_ST_GetTextModeMemoryPtr(void)
{
	return g_textMemory;
}


bool BE_ST_HostGfx_CanToggleAspectRatio(void)
{
	return false;
}

bool BE_ST_HostGfx_GetAspectRatioToggle(void)
{
	return false;
}

void BE_ST_HostGfx_SetAspectRatioToggle(bool aspectToggle)
{
}

bool BE_ST_HostGfx_CanToggleFullScreen(void)
{
	return false;
}

bool BE_ST_HostGfx_GetFullScreenToggle(void)
{
	return false;
}

void BE_ST_HostGfx_SetFullScreenToggle(bool fullScreenToggle)
{
}

#ifdef BE_ST_SDL_ENABLE_ABSMOUSEMOTION_SETTING
void BE_ST_HostGfx_SetAbsMouseCursorToggle(bool cursorToggle)
{
}
#endif


void BE_ST_SetBorderColor(uint8_t color)
{
	D(bug("%s(%d)\n", __FUNCTION__, color));

#ifdef REFKEEN_VER_CAT3D
	if (color == 3)
		color = 0;
#endif

	BEL_ST_SetPalette(1, 0, &color);
}

void BE_ST_EGASetPaletteAndBorder(const uint8_t *palette)
{
	D(bug("%s(%p)\n", __FUNCTION__, palette));

	BEL_ST_SetPalette(16, 0, palette);
	BE_ST_SetBorderColor(palette[16]);
}

void BE_ST_EGASetPelPanning(uint8_t panning)
{
	D(bug("%s(%u)\n", __FUNCTION__, panning));

	if (g_sdlPelPanning != panning)
	{
		g_sdlPelPanning = panning;
		g_amigaScreen->ViewPort.RasInfo->RxOffset = g_sdlPelPanning;
		ScrollVPort(&g_amigaScreen->ViewPort);
	}
}

void BE_ST_EGASetLineWidth(uint8_t widthInBytes)
{
	if (widthInBytes != g_sdlLineWidth)
	{
		g_sdlLineWidth = widthInBytes;
		BEL_ST_ReopenScreen();
	}
}

static void BEL_ST_FreeCopList(void)
{
#ifdef USE_SPLITSCREEN
	CloseScreen(g_splitScreen);
	g_splitScreen = NULL;
#else
	if (g_amigaScreen->ViewPort.UCopIns != NULL)
	{
		FreeVPortCopLists(&g_amigaScreen->ViewPort);
		RemakeDisplay();
	}
#endif
}

#define BPLPTH(p) (*((UWORD *)&custom.bplpt[0] + (2 * (p))))
#define BPLPTL(p) (*((UWORD *)&custom.bplpt[0] + (2 * (p)) + 1))
#define HIWORD(x) ((ULONG)(x)>>16 & 0xffff)
#define LOWORD(x) ((ULONG)(x) & 0xffff)

static void BEL_ST_AddSplitlineCopList(uint16_t splitline)
{
#ifdef USE_SPLITSCREEN
	static struct BitMap bm;
	BEL_ST_PrepareBitmap(&bm, 0);

	g_splitScreen = OpenScreenTags(NULL, 
		//SA_DisplayID, modeid,
		//SA_DClip, (ULONG)&rect,
		SA_Width, 8*g_sdlLineWidth,
		//SA_Height, g_sdlTexHeight,
		SA_Height, g_sdlTexHeight - splitline,
		SA_Depth, 4,
		SA_ShowTitle, FALSE,
		SA_Quiet, TRUE,
		SA_Draggable, FALSE,
		SA_BitMap, (IPTR)&bm,
		SA_Type, CUSTOMSCREEN,
		SA_Parent, (IPTR)g_amigaScreen,
		SA_Top, splitline,
		TAG_DONE);

	//ScreenToFront(g_amigaScreen);
	BEL_ST_SetPalette(16, 0, NULL);
#else
	int i;

	if ((ucl = AllocMem(sizeof(struct UCopList), MEMF_PUBLIC|MEMF_CLEAR)))
	{
		CINIT(ucl, 16);
		CWAIT(ucl, splitline, 0);
		//for (i = 0; i < 4; i++)
		for(i = 4; i--; ) // fixes DblPAL and DblNTSC modes
		{
			CMOVE(ucl, BPLPTH(i), HIWORD(&g_sdlVidMem->egaGfx[i][0]));
			CMOVE(ucl, BPLPTL(i), LOWORD(&g_sdlVidMem->egaGfx[i][0]));
		}
		//CMOVE(ucl, custom.bplcon1, 0x0000); // fine scroll
		CEND(ucl);

		Forbid();
		g_amigaScreen->ViewPort.UCopIns = ucl;
		Permit();

		VideoControlTags(g_amigaScreen->ViewPort.ColorMap, VTAG_USERCLIP_SET, NULL, VTAG_END_CM);
		RethinkDisplay();
	}
#endif
}

void BE_ST_EGASetSplitScreen(int16_t linenum)
{
	// VGA only for now (200-lines graphics modes)
	if (g_sdlTexHeight == GFX_TEX_HEIGHT)
	{
		// Because 200-lines modes are really double-scanned to 400,
		// a linenum of x was originally replaced with 2x-1 in id_vw.c.
		// In practice it should've probably been 2x+1, and this is how
		// we "correct" it here (one less black line in Catacomb Abyss
		// before gameplay begins in a map, above the status bar).
		g_sdlSplitScreenLine = linenum/2;
	}
	else
		g_sdlSplitScreenLine = linenum;

	D(bug("g_sdlSplitScreenLine %d\n", g_sdlSplitScreenLine));

	if (g_sdlSplitScreenLine > 0)
	{
		BEL_ST_AddSplitlineCopList(g_sdlSplitScreenLine+1);
	}
	else
	{
		BEL_ST_FreeCopList();
	}
}

void BE_ST_EGAUpdateGFXByteInPlane(uint16_t destOff, uint8_t srcVal, uint16_t planeNum)
{
	g_sdlVidMem->egaGfx[planeNum][destOff] = srcVal;
}

// Based on BE_Cross_LinearToWrapped_MemCopy
static void BEL_ST_LinearToEGAPlane_MemCopy(uint8_t *planeDstPtr, uint16_t planeDstOff, const uint8_t *linearSrc, uint16_t num)
{
	uint16_t bytesToEnd = 0x10000-planeDstOff;
	if (num <= bytesToEnd)
	{
		memcpy(planeDstPtr+planeDstOff, linearSrc, num);
	}
	else
	{
		memcpy(planeDstPtr+planeDstOff, linearSrc, bytesToEnd);
		memcpy(planeDstPtr, linearSrc+bytesToEnd, num-bytesToEnd);
	}
}

// Based on BE_Cross_WrappedToLinear_MemCopy
static void BEL_ST_EGAPlaneToLinear_MemCopy(uint8_t *linearDst, const uint8_t *planeSrcPtr, uint16_t planeSrcOff, uint16_t num)
{
	uint16_t bytesToEnd = 0x10000-planeSrcOff;
	if (num <= bytesToEnd)
	{
		memcpy(linearDst, planeSrcPtr+planeSrcOff, num);
	}
	else
	{
		memcpy(linearDst, planeSrcPtr+planeSrcOff, bytesToEnd);
		memcpy(linearDst+bytesToEnd, planeSrcPtr, num-bytesToEnd);
	}
}

// Based on BE_Cross_WrappedToWrapped_MemCopy
static void BEL_ST_EGAPlaneToEGAPlane_MemCopy(uint8_t *planeCommonPtr, uint16_t planeDstOff, uint16_t planeSrcOff, uint16_t num)
{
	uint16_t srcBytesToEnd = 0x10000-planeSrcOff;
	uint16_t dstBytesToEnd = 0x10000-planeDstOff;
	if (num <= srcBytesToEnd)
	{
		// Source is linear: Same as BE_Cross_LinearToWrapped_MemCopy here
		if (num <= dstBytesToEnd)
		{
			memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, num);
		}
		else
		{
			memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, dstBytesToEnd);
			memcpy(planeCommonPtr, planeCommonPtr+planeSrcOff+dstBytesToEnd, num-dstBytesToEnd);
		}
	}
	// Otherwise, check if at least the destination is linear
	else if (num <= dstBytesToEnd)
	{
		// Destination is linear: Same as BE_Cross_WrappedToLinear_MemCopy, non-linear source
		memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, srcBytesToEnd);
		memcpy(planeCommonPtr+planeDstOff+srcBytesToEnd, planeCommonPtr, num-srcBytesToEnd);
	}
	// BOTH buffers have wrapping. We don't check separately if
	// srcBytesToEnd==dstBytesToEnd (in such a case planeDstOff==planeSrcOff...)
	else if (srcBytesToEnd <= dstBytesToEnd)
	{
		memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, srcBytesToEnd);
		memcpy(planeCommonPtr+planeDstOff+srcBytesToEnd, planeCommonPtr, dstBytesToEnd-srcBytesToEnd);
		memcpy(planeCommonPtr, planeCommonPtr+(dstBytesToEnd-srcBytesToEnd), num-dstBytesToEnd);
	}
	else // srcBytesToEnd > dstBytesToEnd
	{
		memcpy(planeCommonPtr+planeDstOff, planeCommonPtr+planeSrcOff, dstBytesToEnd);
		memcpy(planeCommonPtr, planeCommonPtr+planeSrcOff+dstBytesToEnd, srcBytesToEnd-dstBytesToEnd);
		memcpy(planeCommonPtr+(srcBytesToEnd-dstBytesToEnd), planeCommonPtr, num-srcBytesToEnd);
	}
}

// A similar analogue of memset
static void BEL_ST_EGAPlane_MemSet(uint8_t *planeDstPtr, uint16_t planeDstOff, uint8_t value, uint16_t num)
{
	uint16_t bytesToEnd = 0x10000-planeDstOff;
	if (num <= bytesToEnd)
	{
		memset(planeDstPtr+planeDstOff, value, num);
	}
	else
	{
		memset(planeDstPtr+planeDstOff, value, bytesToEnd);
		memset(planeDstPtr, value, num-bytesToEnd);
	}
}

// VW_MemToScreen_EGA
void BE_ST_EGAUpdateGFXBufferInPlane(uint16_t destOff, const uint8_t *srcPtr, uint16_t num, uint16_t planeNum)
{
	BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[planeNum], destOff, srcPtr, num);
}

void BE_ST_EGAUpdateGFXByteInAllPlanesScrToScr(uint16_t destOff, uint16_t srcOff)
{
	g_sdlVidMem->egaGfx[0][destOff] = g_sdlVidMem->egaGfx[0][srcOff];
	g_sdlVidMem->egaGfx[1][destOff] = g_sdlVidMem->egaGfx[1][srcOff];
	g_sdlVidMem->egaGfx[2][destOff] = g_sdlVidMem->egaGfx[2][srcOff];
	g_sdlVidMem->egaGfx[3][destOff] = g_sdlVidMem->egaGfx[3][srcOff];
}

// Same as BE_ST_EGAUpdateGFXByteInAllPlanesScrToScr but with a specific plane (added for Catacomb Abyss vanilla bug reproduction/workaround)
void BE_ST_EGAUpdateGFXByteInPlaneScrToScr(uint16_t destOff, uint16_t srcOff, uint16_t planeNum)
{
	g_sdlVidMem->egaGfx[planeNum][destOff] = g_sdlVidMem->egaGfx[planeNum][srcOff];
}

// Same as BE_ST_EGAUpdateGFXByteInAllPlanesScrToScr but picking specific bits out of each byte
void BE_ST_EGAUpdateGFXBitsInAllPlanesScrToScr(uint16_t destOff, uint16_t srcOff, uint8_t bitsMask)
{
	g_sdlVidMem->egaGfx[0][destOff] = (g_sdlVidMem->egaGfx[0][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[0][srcOff] & bitsMask);
	g_sdlVidMem->egaGfx[1][destOff] = (g_sdlVidMem->egaGfx[1][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[1][srcOff] & bitsMask);
	g_sdlVidMem->egaGfx[2][destOff] = (g_sdlVidMem->egaGfx[2][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[2][srcOff] & bitsMask);
	g_sdlVidMem->egaGfx[3][destOff] = (g_sdlVidMem->egaGfx[3][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[3][srcOff] & bitsMask);
}

void BE_ST_EGAUpdateGFXBufferInAllPlanesScrToScr(uint16_t destOff, uint16_t srcOff, uint16_t num)
{
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[0], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[1], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[2], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[3], destOff, srcOff, num);
}

uint8_t BE_ST_EGAFetchGFXByteFromPlane(uint16_t destOff, uint16_t planenum)
{
	return g_sdlVidMem->egaGfx[planenum][destOff];
}

// VW_ScreenToMem_EGA
void BE_ST_EGAFetchGFXBufferFromPlane(uint8_t *destPtr, uint16_t srcOff, uint16_t num, uint16_t planenum)
{
	BEL_ST_EGAPlaneToLinear_MemCopy(destPtr, g_sdlVidMem->egaGfx[planenum], srcOff, num);
}

void BE_ST_EGAUpdateGFXBitsFrom4bitsPixel(uint16_t destOff, uint8_t color, uint8_t bitsMask)
{
	g_sdlVidMem->egaGfx[0][destOff] = (g_sdlVidMem->egaGfx[0][destOff] & ~bitsMask) | (-(color & 1) & bitsMask);
	g_sdlVidMem->egaGfx[1][destOff] = (g_sdlVidMem->egaGfx[1][destOff] & ~bitsMask) | (-((color & 2) >> 1) & bitsMask);
	g_sdlVidMem->egaGfx[2][destOff] = (g_sdlVidMem->egaGfx[2][destOff] & ~bitsMask) | (-((color & 4) >> 2) & bitsMask);
	g_sdlVidMem->egaGfx[3][destOff] = (g_sdlVidMem->egaGfx[3][destOff] & ~bitsMask) | (-((color & 8) >> 3) & bitsMask);
}

void BE_ST_EGAUpdateGFXBufferFrom4bitsPixel(uint16_t destOff, uint8_t color, uint16_t count)
{
#if 1
	uint8_t plane0color = -(color & 1);
	uint8_t plane1color = -((color & 2) >> 1);
	uint8_t plane2color = -((color & 4) >> 2);
	uint8_t plane3color = -((color & 8) >> 3);
	uint8_t *plane0 = (uint8_t *)&g_sdlVidMem->egaGfx[0][destOff];
	uint8_t *plane1 = (uint8_t *)&g_sdlVidMem->egaGfx[1][destOff];
	uint8_t *plane2 = (uint8_t *)&g_sdlVidMem->egaGfx[2][destOff];
	uint8_t *plane3 = (uint8_t *)&g_sdlVidMem->egaGfx[3][destOff];

	for (int loopVar = 0; loopVar < count; ++loopVar)
	{
		*plane0++ = plane0color;
		*plane1++ = plane1color;
		*plane2++ = plane2color;
		*plane3++ = plane3color;
	}
#else
	BEL_ST_EGAPlane_MemSet(g_sdlVidMem->egaGfx[0], destOff, (uint8_t)(-(color&1)), count);
	BEL_ST_EGAPlane_MemSet(g_sdlVidMem->egaGfx[1], destOff, (uint8_t)(-((color&2)>>1)), count);
	BEL_ST_EGAPlane_MemSet(g_sdlVidMem->egaGfx[2], destOff, (uint8_t)(-((color&4)>>2)), count);
	BEL_ST_EGAPlane_MemSet(g_sdlVidMem->egaGfx[3], destOff, (uint8_t)(-((color&8)>>3)), count);
#endif
}

void BE_ST_EGAXorGFXByteByPlaneMask(uint16_t destOff, uint8_t srcVal, uint16_t planeMask)
{
	if (planeMask & 1)
		g_sdlVidMem->egaGfx[0][destOff] ^= srcVal;
	if (planeMask & 2)
		g_sdlVidMem->egaGfx[1][destOff] ^= srcVal;
	if (planeMask & 4)
		g_sdlVidMem->egaGfx[2][destOff] ^= srcVal;
	if (planeMask & 8)
		g_sdlVidMem->egaGfx[3][destOff] ^= srcVal;
}

void BE_ST_CGAUpdateGFXBufferFromWrappedMem(const uint8_t *segPtr, const uint8_t *offInSegPtr, uint16_t byteLineWidth)
{
}

void BE_ST_SetScreenMode(int mode)
{
	D(bug("%s(%x)\n", __FUNCTION__, mode));

	if (g_sdlScreenMode == mode)
		return;

	switch (mode)
	{
	case 3:
#ifdef ENABLE_TEXT
		/*g_sdlTexWidth = VGA_TXT_TEX_WIDTH;
		g_sdlTexHeight = VGA_TXT_TEX_HEIGHT;*/
		g_sdlTxtColor = 7;
		g_sdlTxtBackground = 0;
		g_sdlTxtCursorPosX = g_sdlTxtCursorPosY = 0;
		BE_ST_clrscr();
		g_sdlTexWidth = 2*GFX_TEX_WIDTH;
#ifdef TEXT_CGA_FONT
		g_sdlTexHeight = GFX_TEX_HEIGHT;
#else
		g_sdlTexHeight = 2*GFX_TEX_HEIGHT;
#endif
		g_sdlLineWidth = 80;
		g_sdlSplitScreenLine = -1;
		// TODO should the contents of A0000 be backed up?
		memset(g_sdlVidMem->egaGfx, 0, sizeof(g_sdlVidMem->egaGfx));
		memset(g_textMemoryOld, 0, sizeof(g_textMemoryOld));
		BEL_ST_ReopenScreen();
#else
		BEL_ST_CloseScreen();
#endif
		break;
	case 4:
		// CGA? yuck!
		BE_ST_HandleExit(0);
		break;
	case 0xD:
		g_sdlTexWidth = GFX_TEX_WIDTH;
		g_sdlTexHeight = GFX_TEX_HEIGHT;
		g_sdlLineWidth = 40;
		g_sdlSplitScreenLine = -1;
		// HACK: Looks like this shouldn't be done if changing gfx->gfx
		if (g_sdlScreenMode != 0xE)
		{
			memset(g_sdlVidMem->egaGfx, 0, sizeof(g_sdlVidMem->egaGfx));
		}
		BEL_ST_ReopenScreen();
		break;
	case 0xE:
		g_sdlTexWidth = 2*GFX_TEX_WIDTH;
		g_sdlTexHeight = GFX_TEX_HEIGHT;
		g_sdlLineWidth = 80;
		g_sdlSplitScreenLine = -1;
		// HACK: Looks like this shouldn't be done if changing gfx->gfx
		if (g_sdlScreenMode != 0xD)
		{
			memset(g_sdlVidMem->egaGfx,  0, sizeof(g_sdlVidMem->egaGfx));
		}
		BEL_ST_ReopenScreen();
		break;
	}
	g_sdlScreenMode = mode;
}

void BE_ST_textcolor(int color)
{
#ifdef ENABLE_TEXT
	g_sdlTxtColor = color;
#endif
}

void BE_ST_textbackground(int color)
{
#ifdef ENABLE_TEXT
	g_sdlTxtBackground = color;
#endif
}

void BE_ST_clrscr(void)
{
#ifdef ENABLE_TEXT
	uint8_t *currMemByte = g_textMemory;
	for (int i = 0; i < 2*TXT_COLS_NUM*TXT_ROWS_NUM; ++i)
	{
		*(currMemByte++) = ' ';
		*(currMemByte++) = g_sdlTxtColor | (g_sdlTxtBackground << 4);
	}
	BEL_ST_UpdateHostDisplay();
#endif
}

void BE_ST_MoveTextCursorTo(int x, int y)
{
#ifdef ENABLE_TEXT
	g_sdlTxtCursorPosX = x;
	g_sdlTxtCursorPosY = y;
	BEL_ST_UpdateHostDisplay();
#endif
}

void BE_ST_ToggleTextCursor(bool isEnabled)
{
#ifdef ENABLE_TEXT
	g_sdlTxtCursorEnabled = isEnabled;
#endif
}

#ifdef ENABLE_TEXT
static uint8_t *BEL_ST_printchar(uint8_t *currMemByte, char ch, bool iscolored, bool requirecrchar)
{
	if (ch == '\t')
	{
		int nextCursorPosX = (g_sdlTxtCursorPosX & ~7) + 8;
		for (; g_sdlTxtCursorPosX != nextCursorPosX; ++g_sdlTxtCursorPosX)
		{
			*(currMemByte++) = ' ';
			*currMemByte = iscolored ? (g_sdlTxtColor | (g_sdlTxtBackground << 4)) : *currMemByte;
			++currMemByte;
		}
	}
	else if (ch == '\r')
	{
		if (!requirecrchar)
			return currMemByte; // Do nothing

		g_sdlTxtCursorPosX = 0; // Carriage return
		currMemByte = g_textMemory + 2*TXT_COLS_NUM*g_sdlTxtCursorPosY;
	}
	else if (ch == '\n')
	{
		if (!requirecrchar)
		{
			g_sdlTxtCursorPosX = 0; // Carriage return
		}
		++g_sdlTxtCursorPosY;
		currMemByte = g_textMemory + 2*(g_sdlTxtCursorPosX+TXT_COLS_NUM*g_sdlTxtCursorPosY);
	}
	else
	{
		*(currMemByte++) = ch;
		*currMemByte = iscolored ? (g_sdlTxtColor | (g_sdlTxtBackground << 4)) : *currMemByte;
		++currMemByte;
		++g_sdlTxtCursorPosX;
	}

	// Go to next line
	if (g_sdlTxtCursorPosX == TXT_COLS_NUM)
	{
		g_sdlTxtCursorPosX = 0; // Carriage return
		++g_sdlTxtCursorPosY; // Line feed
		currMemByte = g_textMemory + 2*TXT_COLS_NUM*g_sdlTxtCursorPosY;
	}
	// Shift screen by one line
	if (g_sdlTxtCursorPosY == TXT_ROWS_NUM)
	{
		--g_sdlTxtCursorPosY;
		// Scroll one line down
		uint8_t lastAttr = g_textMemory[sizeof(g_textMemory)-1];
		memmove(g_textMemory, g_textMemory+2*TXT_COLS_NUM, sizeof(g_textMemory)-2*TXT_COLS_NUM);
		currMemByte = g_textMemory+sizeof(g_textMemory)-2*TXT_COLS_NUM;
		// New empty line
		for (int i = 0; i < TXT_COLS_NUM; ++i)
		{
			*(currMemByte++) = ' ';
			*(currMemByte++) = lastAttr;
		}
		currMemByte -= 2*TXT_COLS_NUM; // Go back to beginning of line
	}

	return currMemByte;
}
#endif

void BE_ST_puts(const char *str)
{
#ifdef ENABLE_TEXT
	uint8_t *currMemByte = g_textMemory + 2*(g_sdlTxtCursorPosX+TXT_COLS_NUM*g_sdlTxtCursorPosY);
	for (; *str; ++str)
	{
		currMemByte = BEL_ST_printchar(currMemByte, *str, true, false);
	}
	BEL_ST_printchar(currMemByte, '\n', true, false);

	BEL_ST_UpdateHostDisplay();
#else
	puts(str);
#endif
}

static void BEL_ST_vprintf_impl(const char *format, va_list args, bool iscolored, bool requirecrchar)
{
	vprintf(format, args);
#ifdef ENABLE_TEXT
	uint8_t *currMemByte = g_textMemory + 2*(g_sdlTxtCursorPosX+TXT_COLS_NUM*g_sdlTxtCursorPosY);
	while (*format)
	{
		if (*format == '%')
		{
			switch (*(++format))
			{
			case '%':
				currMemByte = BEL_ST_printchar(currMemByte, '%', iscolored, requirecrchar);
				break;
			case 's':
			{
				for (const char *str = va_arg(args, char *); *str; ++str)
				{
					currMemByte = BEL_ST_printchar(currMemByte, *str, iscolored, requirecrchar);
				}
				break;
			}
			default:
			{
				// Do NOT constify this cause of hack...
				char errorMsg[] = "BEL_ST_vprintf_impl: Unsupported format specifier flag: X\n";
				errorMsg[strlen(errorMsg)-2] = *format; // Hack
				BE_ST_ExitWithErrorMsg(errorMsg);
			}
			}
		}
		else
		{
			currMemByte = BEL_ST_printchar(currMemByte, *format, iscolored, requirecrchar);
		}
		++format;
	}
	BEL_ST_UpdateHostDisplay();
/*#else
	vprintf(format, args);*/
#endif
}

void BE_ST_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	BEL_ST_vprintf_impl(format, args, false, false);
	va_end(args);
}

void BE_ST_cprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	BEL_ST_vprintf_impl(format, args, true, true);
	va_end(args);
}

void BE_ST_vprintf(const char *format, va_list args)
{
	BEL_ST_vprintf_impl(format, args, false, false);
}

#ifdef ENABLE_TEXT
static void BEL_ST_DrawChar(struct RastPort *rp, int x, int y, bool cursor)
{
	uint8_t TmpChar;
	uint8_t TmpFGColor;
	uint8_t TmpBGColor;
	int sX, sY;
	uint8_t *ptr;

	ptr = &g_textMemory[(y * TXT_COLS_NUM + x)*2];
	TmpChar = *ptr++;
	TmpFGColor = *ptr & 0x0f;
	TmpBGColor = (*ptr >> 4) & 0x07;
	sX = x * 8;
	sY = y * FONT_HEIGHT;

	SetABPenDrMd(rp, TmpFGColor, TmpBGColor, JAM2);
	BltTemplate((PLANEPTR)&g_vgaFont[TmpChar * FONT_HEIGHT], 0, 2, rp, sX, sY, 8, FONT_HEIGHT);

	/*if (cursor)
	{
		Move(rp, sX, sY + 14);
		Draw(rp, sX + 7, sY + 14);
		Move(rp, sX, sY + 15);
		Draw(rp, sX + 7, sY + 15);
	}*/
}
#endif

void BEL_ST_UpdateHostDisplay(void)
{
#ifdef ENABLE_TEXT
	int bufCounter = 0;
	if (g_sdlScreenMode == 3) // Text mode
	{
		for (int currCharY = 0; currCharY < TXT_ROWS_NUM; ++currCharY)
		{
			for (int currCharX = 0; currCharX < TXT_COLS_NUM; ++currCharX)
			{
				if (g_textMemoryOld[bufCounter] != g_textMemory[bufCounter] || g_textMemoryOld[bufCounter+1] != g_textMemory[bufCounter+1])
				{
					BEL_ST_DrawChar(&g_amigaScreen->RastPort, currCharX, currCharY, (currCharX == g_sdlTxtCursorPosX && currCharY == g_sdlTxtCursorPosY));
				}
				bufCounter+=2;
			}
		}
	}
	memcpy(g_textMemoryOld, g_textMemory, sizeof(g_textMemoryOld));
#endif
}

// unrolled functions for Keen Dreams
#ifdef REFKEEN_VER_KDREAMS
#define IS_WORD_ALIGNED(x) (((intptr_t)(x) & 1) == 0)

#define SCR_COPY_WORD \
	*(uint16_t *)&g_sdlVidMem->egaGfx[0][destOff] = *(uint16_t *)&g_sdlVidMem->egaGfx[0][srcOff]; \
	*(uint16_t *)&g_sdlVidMem->egaGfx[1][destOff] = *(uint16_t *)&g_sdlVidMem->egaGfx[1][srcOff]; \
	*(uint16_t *)&g_sdlVidMem->egaGfx[2][destOff] = *(uint16_t *)&g_sdlVidMem->egaGfx[2][srcOff]; \
	*(uint16_t *)&g_sdlVidMem->egaGfx[3][destOff] = *(uint16_t *)&g_sdlVidMem->egaGfx[3][srcOff]; \
	destOff += 2; \
	srcOff += 2;

#define SCR_COPY_ROW_ALIGNED \
	SCR_COPY_WORD \
	srcOff += SCREENWIDTH_EGA-2; \
	destOff += SCREENWIDTH_EGA-2;

#define SRC_COPY_PLANES_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED \
	SCR_COPY_ROW_ALIGNED

#define SCR_COPY_BYTE \
	g_sdlVidMem->egaGfx[0][destOff] = g_sdlVidMem->egaGfx[0][srcOff]; \
	g_sdlVidMem->egaGfx[1][destOff] = g_sdlVidMem->egaGfx[1][srcOff]; \
	g_sdlVidMem->egaGfx[2][destOff] = g_sdlVidMem->egaGfx[2][srcOff]; \
	g_sdlVidMem->egaGfx[3][destOff] = g_sdlVidMem->egaGfx[3][srcOff]; \
	destOff++; \
	srcOff++;

#define SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_BYTE \
	SCR_COPY_BYTE \
	srcOff += SCREENWIDTH_EGA-2; \
	destOff += SCREENWIDTH_EGA-2;

#define SRC_COPY_PLANES_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED \
	SCR_COPY_ROW_UNALIGNED

void BE_ST_EGADrawTile16ScrToScr(int destOff, int srcOff)
{
	if (IS_WORD_ALIGNED(destOff) && IS_WORD_ALIGNED(srcOff))
	{
		SRC_COPY_PLANES_ALIGNED
		return;
	}

	SRC_COPY_PLANES_UNALIGNED
}

#define MEM_START_PLANE(p) \
	int egaDestOff##p = destOff;

#define MEM_COPY_WORD(p) \
	*(uint16_t *)&g_sdlVidMem->egaGfx[p][egaDestOff##p] = *(uint16_t *)srcPtr; \
	egaDestOff##p += 2; \
	srcPtr += 2;

#define MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_WORD(p) \
	egaDestOff##p += SCREENWIDTH_EGA-2;

#define MEM_COPY_PLANE_ALIGNED(p) \
	MEM_START_PLANE(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p) \
	MEM_COPY_ROW_ALIGNED(p)

#define MEM_COPY_BYTE(p) \
	g_sdlVidMem->egaGfx[p][egaDestOff##p++] = *srcPtr++;

#define MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_BYTE(p) \
	MEM_COPY_BYTE(p) \
	egaDestOff##p += SCREENWIDTH_EGA-2;

#define MEM_COPY_PLANE_UNALIGNED(p) \
	MEM_START_PLANE(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p) \
	MEM_COPY_ROW_UNALIGNED(p)

void BE_ST_EGADrawTile16MemToScr(int destOff, const uint8_t *srcPtr)
{
	if (IS_WORD_ALIGNED(destOff))
	{
		MEM_COPY_PLANE_ALIGNED(0)
		MEM_COPY_PLANE_ALIGNED(1)
		MEM_COPY_PLANE_ALIGNED(2)
		MEM_COPY_PLANE_ALIGNED(3)
		return;
	}
	MEM_COPY_PLANE_UNALIGNED(0)
	MEM_COPY_PLANE_UNALIGNED(1)
	MEM_COPY_PLANE_UNALIGNED(2)
	MEM_COPY_PLANE_UNALIGNED(3)
}

// TODO calculate the mask location from foreScrPtr
#define MEM_MASKED_START_PLANE(p) \
	int egaDestOff##p = destOff; \
	const uint8_t *maskPtr##p = foreSrcPtr; \
	const uint8_t *dataPtr##p = &foreSrcPtr[2*16*(p+1)];

#define MEM_MASKED_COPY_BYTE(p) \
	g_sdlVidMem->egaGfx[p][egaDestOff##p] = ((*backSrcPtr) & (*maskPtr##p)) | (*dataPtr##p); \
	egaDestOff##p++; \
	backSrcPtr++; \
	maskPtr##p++; \
	dataPtr##p++;

#define MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_BYTE(p) \
	MEM_MASKED_COPY_BYTE(p) \
	egaDestOff##p += SCREENWIDTH_EGA-2;

#define MEM_MASKED_COPY_PLANE_UNALIGNED(p) \
	MEM_MASKED_START_PLANE(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p) \
	MEM_MASKED_COPY_ROW_UNALIGNED(p)

#define MEM_MASKED_COPY_WORD(p) \
	*(uint16_t *)&g_sdlVidMem->egaGfx[p][egaDestOff##p] = ((*(uint16_t *)backSrcPtr) & (*(uint16_t *)maskPtr##p)) | (*(uint16_t *)dataPtr##p); \
	egaDestOff##p += 2; \
	backSrcPtr += 2; \
	maskPtr##p += 2; \
	dataPtr##p += 2;

#define MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_WORD(p) \
	egaDestOff##p += SCREENWIDTH_EGA-2;

#define MEM_MASKED_COPY_PLANE_ALIGNED(p) \
	MEM_MASKED_START_PLANE(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p) \
	MEM_MASKED_COPY_ROW_ALIGNED(p)

void BE_ST_EGADrawTile16MaskedMemToScr(int destOff, const uint8_t *backSrcPtr, const uint8_t *foreSrcPtr)
{
	if (IS_WORD_ALIGNED(destOff))
	{
		MEM_MASKED_COPY_PLANE_ALIGNED(0)
		MEM_MASKED_COPY_PLANE_ALIGNED(1)
		MEM_MASKED_COPY_PLANE_ALIGNED(2)
		MEM_MASKED_COPY_PLANE_ALIGNED(3)
		return;
	}

	MEM_MASKED_COPY_PLANE_UNALIGNED(0)
	MEM_MASKED_COPY_PLANE_UNALIGNED(1)
	MEM_MASKED_COPY_PLANE_UNALIGNED(2)
	MEM_MASKED_COPY_PLANE_UNALIGNED(3)
}

// TODO calculate the mask location from foreScrPtr
#define SRC_MASKED_START_PLANE(p) \
	int egaDestOff##p = destOff; \
	const uint8_t *maskPtr##p = srcPtr; \
	const uint8_t *dataPtr##p = &srcPtr[2*16*(p+1)];

#define SRC_MASKED_COPY_BYTE(p) \
	g_sdlVidMem->egaGfx[p][egaDestOff##p] = (g_sdlVidMem->egaGfx[p][egaDestOff##p] & (*maskPtr##p)) | (*dataPtr##p); \
	egaDestOff##p++; \
	maskPtr##p++; \
	dataPtr##p++;

#define SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_BYTE(p) \
	SRC_MASKED_COPY_BYTE(p) \
	egaDestOff##p += SCREENWIDTH_EGA-2;

#define SRC_MASKED_COPY_PLANE_UNALIGNED(p) \
	SRC_MASKED_START_PLANE(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p) \
	SRC_MASKED_COPY_ROW_UNALIGNED(p)

#define SRC_MASKED_COPY_WORD(p) \
	*(uint16_t *)&g_sdlVidMem->egaGfx[p][egaDestOff##p] = ((*(uint16_t *)&g_sdlVidMem->egaGfx[p][egaDestOff##p]) & (*(uint16_t *)maskPtr##p)) | (*(uint16_t *)dataPtr##p); \
	egaDestOff##p += 2; \
	maskPtr##p += 2; \
	dataPtr##p += 2;

#define SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_WORD(p) \
	egaDestOff##p += SCREENWIDTH_EGA-2;

#define SRC_MASKED_COPY_PLANE_ALIGNED(p) \
	SRC_MASKED_START_PLANE(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p) \
	SRC_MASKED_COPY_ROW_ALIGNED(p)

void BE_ST_EGADrawTile16MaskedSrcToScr(int destOff, const uint8_t *srcPtr)
{
	if (IS_WORD_ALIGNED(destOff))
	{
		SRC_MASKED_COPY_PLANE_ALIGNED(0)
		SRC_MASKED_COPY_PLANE_ALIGNED(1)
		SRC_MASKED_COPY_PLANE_ALIGNED(2)
		SRC_MASKED_COPY_PLANE_ALIGNED(3)
		return;
	}

	SRC_MASKED_COPY_PLANE_UNALIGNED(0)
	SRC_MASKED_COPY_PLANE_UNALIGNED(1)
	SRC_MASKED_COPY_PLANE_UNALIGNED(2)
	SRC_MASKED_COPY_PLANE_UNALIGNED(3)
}
#endif

// common with Catacomb 3D
// TODO: add word/dword aligned copies, unroll loops, etc.
void BE_ST_EGACopyBlockScrToScr(int destOff, int dstLineDelta, int srcOff, int srcLineDelta, int width, int height)
{
	do
	{
		int colsLeft = width;
		do
		{
			g_sdlVidMem->egaGfx[0][destOff] = g_sdlVidMem->egaGfx[0][srcOff];
			g_sdlVidMem->egaGfx[1][destOff] = g_sdlVidMem->egaGfx[1][srcOff];
			g_sdlVidMem->egaGfx[2][destOff] = g_sdlVidMem->egaGfx[2][srcOff];
			g_sdlVidMem->egaGfx[3][destOff] = g_sdlVidMem->egaGfx[3][srcOff];
			++srcOff;
			++destOff;
			--colsLeft;
		} while (colsLeft);
		destOff += dstLineDelta;
		srcOff += srcLineDelta;
		--height;
	} while (height);
}

// used by Catacomb 3D too to draw the hand
// TODO: add unrolled versions for common Keen Dreams sprite sizes (largest: 10 average: 4)
void BE_ST_EGAMaskBlockSrcToSrc(int destOff, int linedelta, uint8_t *srcPtr, int width, int height, int planesize)
{
	//bool aligned = IS_WORD_ALIGNED(destOff);
	do
	{
		int colsLeft = width;
		do
		{
			g_sdlVidMem->egaGfx[0][destOff] = (g_sdlVidMem->egaGfx[0][destOff] & (*srcPtr)) | srcPtr[planesize*1];
			g_sdlVidMem->egaGfx[1][destOff] = (g_sdlVidMem->egaGfx[1][destOff] & (*srcPtr)) | srcPtr[planesize*2];
			g_sdlVidMem->egaGfx[2][destOff] = (g_sdlVidMem->egaGfx[2][destOff] & (*srcPtr)) | srcPtr[planesize*3];
			g_sdlVidMem->egaGfx[3][destOff] = (g_sdlVidMem->egaGfx[3][destOff] & (*srcPtr)) | srcPtr[planesize*4];
			++srcPtr;
			++destOff;
			--colsLeft;
		} while (colsLeft);
		destOff += linedelta;
		--height;
	} while (height);
}

// touch control stubs
void BE_ST_AltControlScheme_InitTouchControlsUI(BE_ST_OnscreenTouchControl *onScreenTouchControls)
{
}
