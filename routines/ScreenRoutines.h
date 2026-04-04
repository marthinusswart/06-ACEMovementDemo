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
void calculateSpriteLocation(int row, int col, int spriteWidth, int spriteHeight, int tilesetWidth, int tilesetHeight, int *spriteX, int *spriteY);
BOOL isValidSpriteLocation(int x, int y, int spriteWidth, int spriteHeight, int screenWidth, int screenHeight);

#endif // SCREENROUTINES_H