#ifndef SCREENROUTINES_H
#define SCREENROUTINES_H

#include <exec/types.h>

APTR GetVBR(void);
void SetInterruptHandler(APTR interrupt);
APTR GetInterruptHandler(void);

void WaitVbl(void);
void WaitLine(USHORT line);
void WaitBlt(void);

USHORT *screenScanDefault(USHORT *copListEnd);

#endif // SCREENROUTINES_H