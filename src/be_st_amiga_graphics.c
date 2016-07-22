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
#define PAGE1START		0x900
#define PAGE2START		0x2000
#define	PAGE3START		0x3700
#define VIEWWIDTH (33*8)
#define VIEWHEIGHT	(18*8)
#else
#define STATUSLEN			0xc80
#define PAGELEN			0x1700
#define PAGE1START	STATUSLEN
#define PAGE2START	(PAGE1START+PAGELEN)
#define PAGE3START	(PAGE2START+PAGELEN)
#define VIEWWIDTH (40*8)
#define VIEWHEIGHT	(15*8)
#endif
#define VIEWX		0
#define VIEWY		0 
#define VIEWXH		(VIEWX+VIEWWIDTH-1)
#define VIEWYH		(VIEWY+VIEWHEIGHT-1)

#define GFX_TEX_WIDTH 320
#define GFX_TEX_HEIGHT 200

#define TXT_COLS_NUM 80
#define TXT_ROWS_NUM 25

#if defined(__AMIGA__)
#define IPTR ULONG
#endif

#define ENABLE_TEXT

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

static union bufferType *g_sdlVidMem;
static uint8_t g_textMemory[TXT_COLS_NUM*TXT_ROWS_NUM*2];
static int g_sdlScreenMode = 3;
static int g_sdlTexWidth, g_sdlTexHeight;
static int16_t g_sdlSplitScreenLine = -1;

static struct BitMap g_screenBitMaps[4];
static int g_bitMapOffsets[4] = {0, PAGE1START, PAGE2START, PAGE3START};
static struct DBufInfo *dbuf = NULL;
static struct UCopList *ucl = NULL;
#ifdef ENABLE_TEXT
static UWORD *g_vgaFont = NULL;
static int g_sdlTxtCursorPosX, g_sdlTxtCursorPosY;
static bool g_sdlTxtCursorEnabled = true;
static int g_sdlTxtColor = 7, g_sdlTxtBackground = 0;
extern const uint8_t g_vga_8x16TextFont[256*8*16];
#endif
static UWORD *g_pointerMem = NULL;
/*static*/unsigned char *g_chunkyBuffer = NULL;
//uint8_t g_chunkyBuffer[VIEWWIDTH * VIEWHEIGHT];

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
}

static void BEL_ST_PrepareBitmap(struct BitMap *BM, uint16_t firstByte)
{
	InitBitMap(BM, 4, g_sdlTexWidth, g_sdlTexHeight);

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

static BOOL BEL_ST_ReopenScreen(void)
{
	// TODO: use only the NewScreen structure
	struct TagItem screenTags[] =
	{
		{SA_Width, g_sdlTexWidth},
		{SA_Height, g_sdlTexHeight},
		{SA_Depth, 4},
		{SA_ShowTitle, FALSE},
		{SA_Quiet, TRUE},
		//{SA_Draggable, FALSE},
		{SA_BitMap, (IPTR)&g_screenBitMaps[0]},
		{TAG_DONE, 0}
	};
	struct NewScreen ns =
	{
		0, 0, -1, -1, 1, // LeftEdge, TopEdge, Width, Height, Depth
		0, 1, // DetailPen, BlockPen
		0, // ViewModes
		CUSTOMSCREEN, // Type
		NULL, // Font
		NULL, // DefaultTitle
		NULL, // Gadgets
		NULL // CustomBitMap
	};

	D(bug("%s()\n", __FUNCTION__));

	BEL_ST_CloseScreen();

	for (int i = 0; i < 4; i++)
	{
		BEL_ST_PrepareBitmap(&g_screenBitMaps[i], g_bitMapOffsets[i]);
	}

#warning HACK!
	if (g_sdlTexWidth > GFX_TEX_WIDTH)
		ns.ViewModes |= HIRES;
	if (g_sdlTexHeight > GFX_TEX_HEIGHT)
		ns.ViewModes |= LACE;

	if ((g_amigaScreen = OpenScreenTagList(&ns, screenTags)))
	{
		if ((dbuf = AllocDBufInfo(&g_amigaScreen->ViewPort)))
		{
#if 0
			dbuf->dbi_SafeMessage.mn_ReplyPort = CreateMsgPort();
			dbuf->dbi_DispMessage.mn_ReplyPort = CreateMsgPort();
#endif

			BEL_ST_SetPalette(16, 0, NULL);

			if ((g_amigaWindow = OpenWindowTags(NULL,
				WA_Flags, WFLG_BACKDROP | WFLG_REPORTMOUSE | WFLG_BORDERLESS | WFLG_ACTIVATE | WFLG_RMBTRAP,
				WA_InnerWidth, g_sdlTexWidth,
				WA_InnerHeight, g_sdlTexHeight,
				WA_CustomScreen, (IPTR)g_amigaScreen,
				TAG_DONE)))
			{
				SetPointer(g_amigaWindow, g_pointerMem, 16, 16, 0, 0);
				return TRUE;
			}
		}
	}

	return FALSE;
}

void BE_ST_InitGfx(void)
{
	g_sdlVidMem = AllocVec(sizeof(*g_sdlVidMem), MEMF_CHIP | MEMF_CLEAR);
#ifdef ENABLE_TEXT
	g_vgaFont = AllocVec(16*16*256, MEMF_CHIP | MEMF_CLEAR);
	UWORD *ptr = g_vgaFont;

	const uint8_t *currCharFontPtr;
	int currCharPixX, currCharPixY;

	for (int currChar = 0; currChar < 256; currChar++)
	{
		currCharFontPtr = g_vga_8x16TextFont + currChar*16*8;
		for (currCharPixY = 0; currCharPixY < 16; ++currCharPixY)
		{
			for (currCharPixX = 0; currCharPixX < 8; ++currCharPixX, ++currCharFontPtr)
			{
				*ptr |= *currCharFontPtr << (15 - currCharPixX);
			}
			ptr++;
		}
	}
#endif
	g_pointerMem = (UWORD *)AllocVec(16 * 16, MEMF_CLEAR | MEMF_CHIP);
	g_chunkyBuffer = (UBYTE *)AllocVec(VIEWWIDTH * VIEWHEIGHT, MEMF_CLEAR | MEMF_FAST);

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
	FreeVec(g_chunkyBuffer);
}

static int BEL_ST_AddressToBitMap(uint16_t crtc)
{
	int curbm;

	switch (crtc)
	{
		case PAGE1START:
			curbm = 1;
			break;
		case PAGE2START:
			curbm = 2;
			break;
		case PAGE3START:
			curbm = 3;
			break;
		default:
			curbm = 0;
	}

	return curbm;
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

	//PrintIText(&g_amigaScreen->RastPort, &WinText, 0, GFX_TEX_HEIGHT);
	InitRastPort(&rp);
	rp.BitMap = &g_screenBitMaps[0];
	PrintIText(&rp, &WinText, x, y);
}

void BE_ST_DebugColor(uint16_t color)
{
	custom.color[0] = color;
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

void BE_ST_DrawChunkyBuffer(uint16_t screenpage)
{
#ifdef KALMS_C2P
	c2p1x1_4_c5_bm(VIEWWIDTH, VIEWHEIGHT, VIEWX, VIEWY, g_chunkyBuffer, &g_screenBitMaps[screenpage+1]);
#else
	struct RastPort rp;
	InitRastPort(&rp);
	rp.BitMap = &g_screenBitMaps[screenpage+1];
	WriteChunkyPixels(&rp, VIEWX, VIEWY, VIEWXH, VIEWYH, g_chunkyBuffer, VIEWWIDTH);
#endif
}

void BE_ST_SetScreenStartAddress(uint16_t crtc)
{
	int curbm;

	D(bug("%s(%d) %x\n", __FUNCTION__, crtc, crtc));

	curbm = BEL_ST_AddressToBitMap(crtc);

#if 0
	while (!GetMsg(dbuf->dbi_DispMessage.mn_ReplyPort))
		WaitPort(dbuf->dbi_DispMessage.mn_ReplyPort);
#endif

	D(bug("changing to bitmap %d\n", curbm));

	//WaitBlit();
	WaitTOF();
	ChangeVPBitMap(&g_amigaScreen->ViewPort, &g_screenBitMaps[curbm], dbuf);
}

uint8_t *BE_ST_GetTextModeMemoryPtr(void)
{
	return g_textMemory;
}

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
}

void BE_ST_EGASetLineWidth(uint8_t widthInBytes)
{
	int newWidth = widthInBytes * 8;
	if (newWidth != g_sdlTexWidth)
	{
		g_sdlTexWidth = newWidth;
		BEL_ST_ReopenScreen();
	}
}

void BEL_ST_FreeCopList(void)
{
	if (g_amigaScreen->ViewPort.UCopIns != NULL)
	{
		FreeVPortCopLists(&g_amigaScreen->ViewPort);
		RemakeDisplay();
	}
}

#define BPLPTH(p) (*((UWORD *)&custom.bplpt[0] + (2 * (p))))
#define BPLPTL(p) (*((UWORD *)&custom.bplpt[0] + (2 * (p)) + 1))

void BEL_ST_AddSplitlineCopList(uint16_t splitline)
{
	struct TagItem uCopTags[] =
	{
		{VTAG_USERCLIP_SET, NULL},
		{VTAG_END_CM, NULL}
	};
	int i;
	long planeaddr;

	if ((ucl = AllocMem(sizeof(struct UCopList), MEMF_PUBLIC|MEMF_CLEAR)))
	{
		CINIT(ucl, 16);
		CWAIT(ucl, splitline, 0);
		for (i = 0; i < 4; i++)
		{
			planeaddr = (long)g_screenBitMaps[0].Planes[i];
			CMOVE(ucl, BPLPTH(i), (long)(planeaddr >> 16) & 0xffff);
			CMOVE(ucl, BPLPTL(i), (long)planeaddr & 0xffff);
		}
		CEND(ucl);

		Forbid();
		g_amigaScreen->ViewPort.UCopIns = ucl;
		Permit();

		VideoControl(g_amigaScreen->ViewPort.ColorMap, uCopTags);
		RethinkDisplay();
	}
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

void BE_ST_EGAMaskBlock(uint16_t destOff, uint8_t *src, uint16_t linedelta, uint16_t width, uint16_t height, uint16_t planesize)
{
	uint8_t *srcPtr = src;
	uint16_t egaDestOff = destOff; // start at same place in all planes
	uint16_t linesLeft = height; // scan lines to draw
	// draw
	do
	{
		uint16_t colsLeft = width;
		do
		{
			g_sdlVidMem->egaGfx[0][egaDestOff] = (g_sdlVidMem->egaGfx[0][egaDestOff] & (*srcPtr)) | srcPtr[planesize*1];
			g_sdlVidMem->egaGfx[1][egaDestOff] = (g_sdlVidMem->egaGfx[1][egaDestOff] & (*srcPtr)) | srcPtr[planesize*2];
			g_sdlVidMem->egaGfx[2][egaDestOff] = (g_sdlVidMem->egaGfx[2][egaDestOff] & (*srcPtr)) | srcPtr[planesize*3];
			g_sdlVidMem->egaGfx[3][egaDestOff] = (g_sdlVidMem->egaGfx[3][egaDestOff] & (*srcPtr)) | srcPtr[planesize*4];
			++srcPtr;
			++egaDestOff;
			--colsLeft;
		} while (colsLeft);
		egaDestOff += linedelta;
		--linesLeft;
	} while (linesLeft);
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
	uint16_t srcBytesToEnd = 0x10000-srcOff;
	uint16_t dstBytesToEnd = 0x10000-destOff;
	if (num <= srcBytesToEnd && num <= dstBytesToEnd)
	{
		// fast(?) linear case
		memcpy(g_sdlVidMem->egaGfx[0]+destOff, g_sdlVidMem->egaGfx[0]+srcOff, num);
		memcpy(g_sdlVidMem->egaGfx[1]+destOff, g_sdlVidMem->egaGfx[1]+srcOff, num);
		memcpy(g_sdlVidMem->egaGfx[2]+destOff, g_sdlVidMem->egaGfx[2]+srcOff, num);
		memcpy(g_sdlVidMem->egaGfx[3]+destOff, g_sdlVidMem->egaGfx[3]+srcOff, num);
		return;
	}

	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[0], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[1], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[2], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[3], destOff, srcOff, num);
}

uint8_t BE_ST_EGAFetchGFXByteFromPlane(uint16_t destOff, uint16_t planenum)
{
	return g_sdlVidMem->egaGfx[planenum][destOff];
}

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
	uint32_t plane0color = 0;
	uint32_t plane1color = 0;
	uint32_t plane2color = 0;
	uint32_t plane3color = 0;
	uint32_t *plane0 = (uint32_t *)&g_sdlVidMem->egaGfx[0][destOff];
	uint32_t *plane1 = (uint32_t *)&g_sdlVidMem->egaGfx[1][destOff];
	uint32_t *plane2 = (uint32_t *)&g_sdlVidMem->egaGfx[2][destOff];
	uint32_t *plane3 = (uint32_t *)&g_sdlVidMem->egaGfx[3][destOff];

	plane0color = -(color & 1);
	plane1color = -((color & 2) >> 1);
	plane2color = -((color & 4) >> 2);
	plane3color = -((color & 8) >> 3);

	for (int loopVar = 0; loopVar < count / 4; ++loopVar)
	{
		*plane0++ = plane0color;
		*plane1++ = plane1color;
		*plane2++ = plane2color;
		*plane3++ = plane3color;
	}

	for (int loopVar = 0; loopVar < count % 4; ++loopVar)
	{
		((uint8_t *)plane0)[loopVar] = (uint8_t)plane0color;
		((uint8_t *)plane1)[loopVar] = (uint8_t)plane1color;
		((uint8_t *)plane2)[loopVar] = (uint8_t)plane2color;
		((uint8_t *)plane3)[loopVar] = (uint8_t)plane3color;
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

void BE_ST_SetScreenMode(int mode)
{
	D(bug("%s(%x)\n", __FUNCTION__, mode));

	if (g_sdlScreenMode == mode)
		return;

	switch (mode)
	{
	case 3:
#ifdef ENABLE_TEXT
		g_sdlTxtColor = 7;
		g_sdlTxtBackground = 0;
		g_sdlTxtCursorPosX = g_sdlTxtCursorPosY = 0;
		BE_ST_clrscr();
		g_sdlTexWidth = GFX_TEX_WIDTH;
		g_sdlTexHeight = 2*GFX_TEX_HEIGHT;
		g_sdlSplitScreenLine = -1;
		// TODO should the contents of A0000 be backed up?
		memset(g_sdlVidMem->egaGfx, 0, sizeof(g_sdlVidMem->egaGfx));
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
	g_sdlTxtColor = color;
}

void BE_ST_textbackground(int color)
{
	g_sdlTxtBackground = color;
}

void BE_ST_clrscr(void)
{
	uint8_t *currMemByte = g_textMemory;
	for (int i = 0; i < 2*TXT_COLS_NUM*TXT_ROWS_NUM; ++i)
	{
		*(currMemByte++) = ' ';
		*(currMemByte++) = g_sdlTxtColor | (g_sdlTxtBackground << 4);
	}
	BEL_ST_UpdateHostDisplay();
}

void BE_ST_MoveTextCursorTo(int x, int y)
{
	g_sdlTxtCursorPosX = x;
	g_sdlTxtCursorPosY = y;
	BEL_ST_UpdateHostDisplay();
}

void BE_ST_ToggleTextCursor(bool isEnabled)
{
	g_sdlTxtCursorEnabled = isEnabled;
}

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
#ifdef ENABLE_TEXT
	vprintf(format, args);
#else
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
	sY = y * 16;

	SetABPenDrMd(rp, TmpFGColor, TmpBGColor, JAM2);
	BltTemplate(&g_vgaFont[TmpChar * 16], 0, 2, rp, sX, sY, 8, 16);

	if (cursor)
	{
		Move(rp, sX, sY + 14);
		Draw(rp, sX + 7, sY + 14);
		Move(rp, sX, sY + 15);
		Draw(rp, sX + 7, sY + 15);
	}
}
#endif

void BEL_ST_UpdateHostDisplay(void)
{
#ifdef ENABLE_TEXT
	if (g_sdlScreenMode == 3) // Text mode
	{
		for (int currCharY = 0; currCharY < TXT_ROWS_NUM; ++currCharY)
		{
			for (int currCharX = 0; currCharX < TXT_COLS_NUM; ++currCharX)
			{
				BEL_ST_DrawChar(&g_amigaScreen->RastPort, currCharX, currCharY, (currCharX == g_sdlTxtCursorPosX && currCharY == g_sdlTxtCursorPosY));
			}
		}
	}
#endif
}
