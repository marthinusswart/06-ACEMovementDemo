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
// Calculates the X and Y pixel coordinates of a sprite within a tileset
void calculateSpriteLocation(int row, int col, int sprite_width, int sprite_height, int tileset_width, int tileset_height, int *sprite_x, int *sprite_y);

#endif // SCREENROUTINES_H