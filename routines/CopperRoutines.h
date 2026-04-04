#ifndef COPPERROUTINES_H
#define COPPERROUTINES_H

#include <exec/types.h>

USHORT *copSetPlanes(UBYTE bplPtrStart, USHORT *copListEnd, const UBYTE **planes, int numPlanes);
USHORT *copWaitXY(USHORT *copListEnd, USHORT x, USHORT i);
USHORT *copWaitY(USHORT *copListEnd, USHORT i);
USHORT *copSetColor(USHORT *copListCurrent, USHORT index, USHORT color);

#endif // COPPERROUTINES_H