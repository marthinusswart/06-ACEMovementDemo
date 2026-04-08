#include "CopperRoutines.h"
#include "support/gcc8_c_support.h"
#include <hardware/custom.h>

USHORT *copSetPlanes(UBYTE bplPtrStart, USHORT *copListEnd, const UBYTE **planes, int numPlanes)
{
    for (int i = 0; i < numPlanes; i++)
    {
        ULONG addr = (ULONG)planes[i];
        *copListEnd++ = (USHORT)(offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR));
        *copListEnd++ = (UWORD)(addr >> 16);
        *copListEnd++ = (USHORT)(offsetof(struct Custom, bplpt[0]) + (i + bplPtrStart) * sizeof(APTR) + 2);
        *copListEnd++ = (UWORD)addr;
    }
    return copListEnd;
}

USHORT *copWaitXY(USHORT *copListEnd, USHORT x, USHORT i)
{
    *copListEnd++ = (USHORT)((i << 8) | (x << 1) | 1); // bit 1 means wait. waits for vertical position x<<8, first raster stop position outside the left
    *copListEnd++ = 0xfffe;
    return copListEnd;
}

USHORT *copWaitY(USHORT *copListEnd, USHORT i)
{
    *copListEnd++ = (USHORT)((i << 8) | 4 | 1); // bit 1 means wait. waits for vertical position x<<8, first raster stop position outside the left
    *copListEnd++ = 0xfffe;
    return copListEnd;
}

USHORT *copSetColor(USHORT *copListCurrent, USHORT index, USHORT color)
{
    *copListCurrent++ = (USHORT)(offsetof(struct Custom, color) + sizeof(UWORD) * index);
    *copListCurrent++ = color;
    return copListCurrent;
}