#include "copper_routines.h"
#include "screen_routines.h"
#include "support/gcc8_c_support.h"
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include <ace/utils/bitmap.h>

short copSetPlanes(UBYTE bplPtrStart, USHORT **copListEnd_out, const UBYTE **planes, int numPlanes)
{
    if (!copListEnd_out || !*copListEnd_out || !planes)
        return -1;

    USHORT *copListEnd = *copListEnd_out;
    for (int i = 0; i < numPlanes; i++)
    {
        ULONG addr = (ULONG)planes[i];
        *copListEnd++ = (USHORT)(offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR));
        *copListEnd++ = (UWORD)(addr >> 16);
        *copListEnd++ = (USHORT)(offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR) + 2);
        *copListEnd++ = (UWORD)addr;
    }
    *copListEnd_out = copListEnd;
    return 0;
}

short copSetColor(USHORT **copListCurrent_out, USHORT index, USHORT color)
{
    if (!copListCurrent_out || !*copListCurrent_out)
        return -1;

    USHORT *copListCurrent = *copListCurrent_out;
    *copListCurrent++ = (USHORT)(offsetof(struct Custom, color) + sizeof(UWORD) * index);
    *copListCurrent++ = color;
    *copListCurrent_out = copListCurrent;
    return 0;
}

int copperListCreate(USHORT **copper_out, tBitMap *screenBuffer,
                     volatile struct Custom *customPtr, const USHORT *colorPalette,
                     USHORT **bplPtrsOut)
{
    if (!copper_out || !screenBuffer || !customPtr || !colorPalette || !bplPtrsOut)
        return -1;

    USHORT *copper = (USHORT *)AllocMem(1024, MEMF_CHIP);
    if (!copper)
        return -1;

    USHORT *copPtr = copper;

    screenScanDefault(&copPtr);

    // enable bitplanes
    *copPtr++ = offsetof(struct Custom, bplcon0);
    *copPtr++ = (0 << 10) /*dual pf*/ | (1 << 9) /*color*/ | ((5) << 12) /*num bitplanes*/;
    *copPtr++ = offsetof(struct Custom, bplcon1); // scrolling
    *copPtr++ = 0;
    *copPtr++ = offsetof(struct Custom, bplcon2); // playfied priority
    *copPtr++ = 1 << 6;                           // 0x24;         //Sprites have priority over playfields

    // set bitplane modulo for PLANAR
    *copPtr++ = offsetof(struct Custom, bpl1mod); // odd planes   1,3,5
    *copPtr++ = 0;
    *copPtr++ = offsetof(struct Custom, bpl2mod); // even  planes 2,4
    *copPtr++ = 0;

    // set bitplane pointers
    const UBYTE *planes[5];
    for (int a = 0; a < 5; a++)
    {
        planes[a] = screenBuffer->Planes[a];
    }
    *bplPtrsOut = copPtr;                // Return pointer so caller can update it in the main loop
    copSetPlanes(0, &copPtr, planes, 5); // INJECT pointers into copper list!

    // set colors
    for (int a = 0; a < 32; a++)
        copSetColor(&copPtr, a, colorPalette[a]);

    // end copper list
    *copPtr++ = 0xffff;
    *copPtr++ = 0xfffe;

    customPtr->cop1lc = (ULONG)copper;
    customPtr->dmacon = DMAF_BLITTER; // disable blitter dma for copjmp bug
    customPtr->copjmp1 = 0x7fff;      // start coppper

    *copper_out = copper;
    return 0;
}