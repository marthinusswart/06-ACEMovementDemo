#ifndef SCREEN_ROUTINES_H
#define SCREEN_ROUTINES_H

#include <exec/types.h>
#include <proto/exec.h>
#include <exec/execbase.h>
#include <hardware/custom.h>
#include <ace/utils/bitmap.h>
#include "support/gcc8_c_support.h"

// Entity IDs for double buffering history trackers
#define PACMAN 0
#define BLUE 1
#define RED 2
#define PINK 3
#define ORANGE 4
#define NUM_ENTITIES 5

typedef struct
{
    int x;
    int y;
} Position;

APTR GetVBR(void);
void SetInterruptHandler(APTR interrupt);
APTR GetInterruptHandler(void);

void WaitVbl(void);
void WaitLine(USHORT line);
void WaitBlt(void);

short screenScanDefault(USHORT **copListEnd_out);
int setupBuffers(tBitMap **screenBuffers, tBitMap **pacmanTiles, tBitMap **background,
				 const UBYTE *tileData, const UBYTE *backgroundData);
void doubleBufferUpdates(tBitMap **screenBuffers, int *frontBufferIdx_out,
						 int *backBufferIdx_out, USHORT *bplPtrs);
// Calculates the X and Y pixel coordinates of a sprite within a tileset
void calculateSpriteLocation(int row, int col, int spriteWidth, int spriteHeight, int tilesetWidth, int tilesetHeight, int *spriteX, int *spriteY);
BOOL isValidSpriteLocation(int x, int y, int spriteWidth, int spriteHeight, int screenWidth, int screenHeight);

#endif // SCREEN_ROUTINES_H