#include <string.h>
//#include "SDL.h"
#include <graphics/gfxmacros.h>
#include <graphics/videocontrol.h>
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
#else
#define STATUSLEN			0xc80
#define PAGELEN			0x1700
#define PAGE1START	STATUSLEN
#define PAGE2START	(PAGE1START+PAGELEN)
#define PAGE3START	(PAGE2START+PAGELEN)
#endif

#define GFX_TEX_WIDTH 320
#define GFX_TEX_HEIGHT 200

#define TXT_COLS_NUM 80
#define TXT_ROWS_NUM 25

#if defined(__AMIGA__)
#define IPTR ULONG
#endif

extern struct Custom custom;


// We can use a union because the memory contents are refreshed on screen mode change
// (well, not on change between modes 0xD and 0xE, both sharing planar A000:0000)
union bufferType {
	uint8_t egaGfx[4][0x10000]; // Contents of A000:0000 (4 planes)
	uint8_t text[TXT_COLS_NUM*TXT_ROWS_NUM*2]; // Textual contents of B800:0000
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

static union bufferType *g_sdlVidMem;
static int g_sdlScreenMode = 3;
static int g_sdlTexWidth, g_sdlTexHeight;
static int16_t g_sdlSplitScreenLine = -1;

static struct BitMap g_screenBitMaps[4];
static int g_bitMapOffsets[4] = {0, PAGE1START, PAGE2START, PAGE3START};
static struct DBufInfo *dbuf = NULL;
static struct UCopList *ucl = NULL;

void BE_ST_MarkGfxForUpdate(void)
{
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
	bug("%s()\n", __FUNCTION__);

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

	bug("%s()\n", __FUNCTION__);

	BEL_ST_CloseScreen();

	for (int i = 0; i < 4; i++)
	{
		BEL_ST_PrepareBitmap(&g_screenBitMaps[i], g_bitMapOffsets[i]);
	}

#warning HACK!
	if (g_sdlTexWidth > GFX_TEX_WIDTH)
		ns.ViewModes = HIRES;

	if ((g_amigaScreen = OpenScreenTagList(&ns, screenTags)))
	{
		if ((dbuf = AllocDBufInfo(&g_amigaScreen->ViewPort)))
		{
#if 0
			dbuf->dbi_SafeMessage.mn_ReplyPort = CreateMsgPort();
			dbuf->dbi_DispMessage.mn_ReplyPort = CreateMsgPort();
#endif

			BEL_ST_SetPalette(16, 0, NULL);

			return TRUE;
		}
	}

	return FALSE;
}

void BE_ST_InitGfx(void)
{
	g_sdlVidMem = AllocVec(sizeof(*g_sdlVidMem), MEMF_CHIP | MEMF_CLEAR);

	BE_ST_SetScreenMode(3);
}

void BE_ST_ShutdownGfx(void)
{
	BEL_ST_CloseScreen();
	FreeVec(g_sdlVidMem);
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

	WaitBlit();
	ChangeVPBitMap(&g_amigaScreen->ViewPort, &g_screenBitMaps[curbm], dbuf);
}

uint8_t *BE_ST_GetTextModeMemoryPtr(void)
{
	return g_sdlVidMem->text;
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
	bug("%s(%p)\n", __FUNCTION__, palette);

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

	bug("g_sdlSplitScreenLine %d\n", g_sdlSplitScreenLine);

	if (g_sdlSplitScreenLine > 0)
	{
		BEL_ST_AddSplitlineCopList(g_sdlSplitScreenLine+1);
	}
	else
	{
		BEL_ST_FreeCopList();
	}
}

void BE_ST_EGAUpdateGFXByte(uint16_t destOff, uint8_t srcVal, uint16_t planeMask)
{
	if (planeMask & 1)
		g_sdlVidMem->egaGfx[0][destOff] = srcVal;
	if (planeMask & 2)
		g_sdlVidMem->egaGfx[1][destOff] = srcVal;
	if (planeMask & 4)
		g_sdlVidMem->egaGfx[2][destOff] = srcVal;
	if (planeMask & 8)
		g_sdlVidMem->egaGfx[3][destOff] = srcVal;
}

// Same as BE_ST_EGAUpdateGFXByte but picking specific bits out of each byte, and WITHOUT plane mask
void BE_ST_EGAUpdateGFXBits(uint16_t destOff, uint8_t srcVal, uint8_t bitsMask)
{
	g_sdlVidMem->egaGfx[0][destOff] = (g_sdlVidMem->egaGfx[0][destOff] & ~bitsMask) | (srcVal & bitsMask);
	g_sdlVidMem->egaGfx[1][destOff] = (g_sdlVidMem->egaGfx[1][destOff] & ~bitsMask) | (srcVal & bitsMask);
	g_sdlVidMem->egaGfx[2][destOff] = (g_sdlVidMem->egaGfx[2][destOff] & ~bitsMask) | (srcVal & bitsMask);
	g_sdlVidMem->egaGfx[3][destOff] = (g_sdlVidMem->egaGfx[3][destOff] & ~bitsMask) | (srcVal & bitsMask);
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

void BE_ST_EGAUpdateGFXBuffer(uint16_t destOff, const uint8_t *srcPtr, uint16_t num, uint16_t planeMask)
{
	if (planeMask & 1)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[0], destOff, srcPtr, num);
	if (planeMask & 2)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[1], destOff, srcPtr, num);
	if (planeMask & 4)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[2], destOff, srcPtr, num);
	if (planeMask & 8)
		BEL_ST_LinearToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[3], destOff, srcPtr, num);
}

void BE_ST_EGAUpdateGFXByteScrToScr(uint16_t destOff, uint16_t srcOff)
{
	g_sdlVidMem->egaGfx[0][destOff] = g_sdlVidMem->egaGfx[0][srcOff];
	g_sdlVidMem->egaGfx[1][destOff] = g_sdlVidMem->egaGfx[1][srcOff];
	g_sdlVidMem->egaGfx[2][destOff] = g_sdlVidMem->egaGfx[2][srcOff];
	g_sdlVidMem->egaGfx[3][destOff] = g_sdlVidMem->egaGfx[3][srcOff];
}

// Same as BE_ST_EGAUpdateGFXByteScrToScr but with plane mask (added for Catacomb Abyss vanilla bug reproduction/workaround)
void BE_ST_EGAUpdateGFXByteWithPlaneMaskScrToScr(uint16_t destOff, uint16_t srcOff, uint16_t planeMask)
{
	if (planeMask & 1)
		g_sdlVidMem->egaGfx[0][destOff] = g_sdlVidMem->egaGfx[0][srcOff];
	if (planeMask & 2)
		g_sdlVidMem->egaGfx[1][destOff] = g_sdlVidMem->egaGfx[1][srcOff];
	if (planeMask & 4)
		g_sdlVidMem->egaGfx[2][destOff] = g_sdlVidMem->egaGfx[2][srcOff];
	if (planeMask & 8)
		g_sdlVidMem->egaGfx[3][destOff] = g_sdlVidMem->egaGfx[3][srcOff];
}

// Same as BE_ST_EGAUpdateGFXByteScrToScr but picking specific bits out of each byte
void BE_ST_EGAUpdateGFXBitsScrToScr(uint16_t destOff, uint16_t srcOff, uint8_t bitsMask)
{
	g_sdlVidMem->egaGfx[0][destOff] = (g_sdlVidMem->egaGfx[0][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[0][srcOff] & bitsMask);
	g_sdlVidMem->egaGfx[1][destOff] = (g_sdlVidMem->egaGfx[1][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[1][srcOff] & bitsMask);
	g_sdlVidMem->egaGfx[2][destOff] = (g_sdlVidMem->egaGfx[2][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[2][srcOff] & bitsMask);
	g_sdlVidMem->egaGfx[3][destOff] = (g_sdlVidMem->egaGfx[3][destOff] & ~bitsMask) | (g_sdlVidMem->egaGfx[3][srcOff] & bitsMask);
}

void BE_ST_EGAUpdateGFXBufferScrToScr(uint16_t destOff, uint16_t srcOff, uint16_t num)
{
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[0], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[1], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[2], destOff, srcOff, num);
	BEL_ST_EGAPlaneToEGAPlane_MemCopy(g_sdlVidMem->egaGfx[3], destOff, srcOff, num);
}

uint8_t BE_ST_EGAFetchGFXByte(uint16_t destOff, uint16_t planenum)
{
	return g_sdlVidMem->egaGfx[planenum][destOff];
}

void BE_ST_EGAFetchGFXBuffer(uint8_t *destPtr, uint16_t srcOff, uint16_t num, uint16_t planenum)
{
	BEL_ST_EGAPlaneToLinear_MemCopy(destPtr, g_sdlVidMem->egaGfx[planenum], srcOff, num);
}

void BE_ST_EGAUpdateGFXPixel4bpp(uint16_t destOff, uint8_t color, uint8_t bitsMask)
{
	for (int currBitNum = 0, currBitMask = 1; currBitNum < 8; ++currBitNum, currBitMask <<= 1)
	{
		if (bitsMask & currBitMask)
		{
			g_sdlVidMem->egaGfx[0][destOff] &= ~currBitMask;
			g_sdlVidMem->egaGfx[0][destOff] |= ((color & 1) << currBitNum);
			g_sdlVidMem->egaGfx[1][destOff] &= ~currBitMask;
			g_sdlVidMem->egaGfx[1][destOff] |= (((color & 2) >> 1) << currBitNum);
			g_sdlVidMem->egaGfx[2][destOff] &= ~currBitMask;
			g_sdlVidMem->egaGfx[2][destOff] |= (((color & 4) >> 2) << currBitNum);
			g_sdlVidMem->egaGfx[3][destOff] &= ~currBitMask;
			g_sdlVidMem->egaGfx[3][destOff] |= (((color & 8) >> 3) << currBitNum);
		}
	}
}

void BE_ST_EGAUpdateGFXPixel4bppRepeatedly(uint16_t destOff, uint8_t color, uint16_t count, uint8_t bitsMask)
{
	for (uint16_t loopVar = 0; loopVar < count; ++loopVar, ++destOff)
	{
		BE_ST_EGAUpdateGFXPixel4bpp(destOff, color, bitsMask);
	}
}

void BE_ST_EGAXorGFXByte(uint16_t destOff, uint8_t srcVal, uint16_t planeMask)
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

// Like BE_ST_EGAXorGFXByte, but:
// - OR instead of XOR.
// - All planes are updated.
// - Only specific bits are updated in each plane's byte.
void BE_ST_EGAOrGFXBits(uint16_t destOff, uint8_t srcVal, uint8_t bitsMask)
{
	g_sdlVidMem->egaGfx[0][destOff] |= (srcVal & bitsMask);
	g_sdlVidMem->egaGfx[1][destOff] |= (srcVal & bitsMask);
	g_sdlVidMem->egaGfx[2][destOff] |= (srcVal & bitsMask);
	g_sdlVidMem->egaGfx[3][destOff] |= (srcVal & bitsMask);
}

void BE_ST_CGAFullUpdateFromWrappedMem(const uint8_t *segPtr, const uint8_t *offInSegPtr, uint16_t byteLineWidth)
{
}

void BE_ST_SetScreenMode(int mode)
{
	bug("%s(%x)\n", __FUNCTION__, mode);

	if (g_sdlScreenMode == mode)
		return;

	switch (mode)
	{
	case 3:
		BEL_ST_CloseScreen();
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
}

void BE_ST_textbackground(int color)
{
}

void BE_ST_clrscr(void)
{
}

void BE_ST_MoveTextCursorTo(int x, int y)
{
}

void BE_ST_ToggleTextCursor(bool isEnabled)
{
}

void BE_ST_puts(const char *str)
{
	puts(str);
}

void BE_ST_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void BE_ST_cprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void BE_ST_vprintf(const char *format, va_list args)
{
	vprintf(format, args);
}

void BEL_ST_UpdateHostDisplay(void)
{
}