#ifndef COPPER_ROUTINES_H
#define COPPER_ROUTINES_H

#include <exec/types.h>
#include <ace/utils/bitmap.h>
#include <hardware/custom.h>

short copSetPlanes(UBYTE bplPtrStart, USHORT **copListEnd_out, const UBYTE **planes, int numPlanes);
short copSetColor(USHORT **copListCurrent_out, USHORT index, USHORT color);
int copperListCreate(USHORT **copper_out, tBitMap *screenBuffer,
                     volatile struct Custom *customPtr, const USHORT *colorPalette,
                     USHORT **bplPtrsOut);

#endif // COPPER_ROUTINES_H