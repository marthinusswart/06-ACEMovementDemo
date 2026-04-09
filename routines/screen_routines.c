#include "screen_routines.h"
#include "copper_routines.h"
#include <proto/graphics.h>
#include <graphics/gfx.h>

extern struct ExecBase *SysBase;
extern volatile APTR VBR;
extern volatile struct Custom *custom;

APTR GetVBR(void)
{
    APTR vbr = 0;
    UWORD getvbr[] = {0x4e7a, 0x0801, 0x4e73}; // MOVEC.L VBR,D0 RTE

    if (SysBase->AttnFlags & AFF_68010)
        vbr = (APTR)Supervisor((ULONG (*)())getvbr);

    return vbr;
}

void SetInterruptHandler(APTR interrupt)
{
    *(volatile APTR *)(((UBYTE *)VBR) + 0x6c) = interrupt;
}

APTR GetInterruptHandler(void)
{
    return *(volatile APTR *)(((UBYTE *)VBR) + 0x6c);
}

// vblank begins at vpos 312 hpos 1 and ends at vpos 25 hpos 1
// vsync begins at line 2 hpos 132 and ends at vpos 5 hpos 18
void WaitVbl(void)
{
    debug_start_idle();
    while (1)
    {
        volatile ULONG vpos = *(volatile ULONG *)0xDFF004;
        vpos &= 0x1ff00;
        if (vpos != (311 << 8))
            break;
    }
    while (1)
    {
        volatile ULONG vpos = *(volatile ULONG *)0xDFF004;
        vpos &= 0x1ff00;
        if (vpos == (311 << 8))
            break;
    }
    debug_stop_idle();
}

void WaitLine(USHORT line)
{
    while (1)
    {
        volatile ULONG vpos = *(volatile ULONG *)0xDFF004;
        if (((vpos >> 8) & 511) == line)
            break;
    }
}

void WaitBlt(void)
{
    UWORD tst = *(volatile UWORD *)&custom->dmaconr; // for compatiblity a1000
    (void)tst;
    while (*(volatile UWORD *)&custom->dmaconr & (1 << 14))
    {
    } // blitter busy wait
}

// set up a 320x256 lowres display
short screenScanDefault(USHORT **copListEnd_out)
{
    if (!copListEnd_out || !*copListEnd_out)
        return -1;

    USHORT *copListEnd = *copListEnd_out;
    const USHORT x = 129;
    const USHORT width = 320;
    const USHORT height = 256;
    const USHORT y = 44;
    const USHORT RES = 8; // 8=lowres,4=hires
    USHORT xstop = x + width;
    USHORT ystop = y + height;
    USHORT fw = (x >> 1) - RES;

    *copListEnd++ = offsetof(struct Custom, ddfstrt);
    *copListEnd++ = fw;
    *copListEnd++ = offsetof(struct Custom, ddfstop);
    *copListEnd++ = fw + (((width >> 4) - 1) << 3);
    *copListEnd++ = offsetof(struct Custom, diwstrt);
    *copListEnd++ = x + (y << 8);
    *copListEnd++ = offsetof(struct Custom, diwstop);
    *copListEnd++ = (xstop - 256) + ((ystop - 256) << 8);

    *copListEnd_out = copListEnd;
    return 0;
}

// Calculates the X and Y pixel coordinates of a sprite within a tileset
void calculateSpriteLocation(int row, int col, int spriteWidth, int spriteHeight, int tilesetWidth, int tilesetHeight, int *spriteX, int *spriteY)
{
    // Multiply row/col by dimensions, and use modulo to safely wrap if out of bounds
    *spriteX = (col * spriteWidth) % tilesetWidth;
    *spriteY = (row * spriteHeight) % tilesetHeight;
}

BOOL isValidSpriteLocation(int x, int y, int spriteWidth, int spriteHeight, int screenWidth, int screenHeight)
{
    return (x >= 0) && (y >= 0) && (x + spriteWidth <= screenWidth) && (y + spriteHeight <= screenHeight);
}

int setupBuffers(tBitMap **screenBuffers, tBitMap **pacmanTiles, tBitMap **background,
				 const UBYTE *tileData, const UBYTE *backgroundData)
{
	// 1. Create PLANAR screen buffers safely using ACE (BMF_DISPLAYABLE forces CHIP RAM)
	screenBuffers[0] = bitmapCreate(320, 256, 5, BMF_CLEAR | BMF_DISPLAYABLE);
	screenBuffers[1] = bitmapCreate(320, 256, 5, BMF_CLEAR | BMF_DISPLAYABLE);

	if (!screenBuffers[0] || !screenBuffers[1])
		return -1;

	// 2. Wrap the INCBIN planar tile data directly in a tBitMap (no memory copy needed!)
	*pacmanTiles = (tBitMap *)AllocMem(sizeof(tBitMap), MEMF_PUBLIC | MEMF_CLEAR);
	if (!*pacmanTiles)
		return -1;

	InitBitMap((struct BitMap *)*pacmanTiles, 5, 320, 320); // Assumes tileset is 320x320
	for (int p = 0; p < 5; p++)
	{
		(*pacmanTiles)->Planes[p] = (PLANEPTR)(tileData + p * (320 / 8) * 320);
	}

	// 3. Wrap the background image data in a tBitMap
	*background = (tBitMap *)AllocMem(sizeof(tBitMap), MEMF_PUBLIC | MEMF_CLEAR);
	if (!*background)
		return -1;

	InitBitMap((struct BitMap *)*background, 5, 320, 256);
	for (int p = 0; p < 5; p++)
	{
		(*background)->Planes[p] = (PLANEPTR)(backgroundData + p * (320 / 8) * 256);
	}

	// Copy the background into the screen buffer initially
	for (int p = 0; p < 5; p++)
	{
		CopyMem((*background)->Planes[p], screenBuffers[0]->Planes[p], (320 / 8) * 256);
		CopyMem((*background)->Planes[p], screenBuffers[1]->Planes[p], (320 / 8) * 256);
	}

	return 0;
}

void doubleBufferUpdates(tBitMap **screenBuffers, int *frontBufferIdx_out,
						 int *backBufferIdx_out, USHORT *bplPtrs)
{
	const UBYTE *planes[5];
	for (int a = 0; a < 5; a++)
	{
		planes[a] = screenBuffers[*backBufferIdx_out]->Planes[a];
	}
	// Safely swap the bitplane pointers in the copper list
	USHORT *tempBplPtr = bplPtrs;
	copSetPlanes(0, &tempBplPtr, planes, 5);

	// Flip buffers for the next frame
	*frontBufferIdx_out = *backBufferIdx_out;
	*backBufferIdx_out = 1 - *frontBufferIdx_out;
}