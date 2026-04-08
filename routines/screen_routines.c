#include "ScreenRoutines.h"
#include <proto/exec.h>
#include <exec/execbase.h>
#include <hardware/custom.h>
#include "support/gcc8_c_support.h"

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
USHORT *screenScanDefault(USHORT *copListEnd)
{
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
    return copListEnd;
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