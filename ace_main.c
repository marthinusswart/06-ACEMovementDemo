#include <ace/types.h>
#include <ace/generic/main.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/key.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/blit.h>
#include <ace/utils/palette.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include "support/gcc8_c_support.h"

// Debug logging macro
// Comment out the next line to disable debug logging
#define DEBUG

#ifdef DEBUG
#define DEBUG_LOG(...) KPrintF(__VA_ARGS__)
#else
#define DEBUG_LOG(...) ((void)0)
#endif

// Library bases - ACE framework initializes these
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;
#include "routines/screen_routines.h"
#include "player/pacman.h"

#ifdef MUSIC
#include "routines/music_routines.h"
#endif

// ACE view/viewport/buffer
tView *g_pView;
tVPort *g_pVPort;
tSimpleBufferManager *g_pBufferManager;

// Game objects
Pacman *pacman;
tBitMap *tPacmanTiles = NULL;

// INCBIN assets
volatile short frameCounter = 0;
INCBIN(colors, "pal/pacman_tiles.pal")
INCBIN_CHIP(pacman_tiles, "bpl/pacman_tiles.bpl")
INCBIN_CHIP(pacman_tiles_mask, "bpl/pacman_tiles_mask.bpl")

static void vblankHandler(void)
{
#ifdef MUSIC
	p61Music();
#endif
	frameCounter++;
}

static void setupPacmanTiles(void)
{
	// Wrap the INCBIN planar tile data directly in a tBitMap
	tPacmanTiles = (tBitMap *)AllocMem(sizeof(tBitMap), MEMF_PUBLIC | MEMF_CLEAR);
	InitBitMap((struct BitMap *)tPacmanTiles, 5, 320, 320);
	for (int p = 0; p < 5; p++)
	{
		tPacmanTiles->Planes[p] = (PLANEPTR)(pacman_tiles + p * (320 / 8) * 320);
	}
}

static void setupPacman(void)
{
	int bobX = 0;
	int bobY = 0;
	DEBUG_LOG("Create Pacman!\n");
	pacman = createPacman(208, 150, 16, 16);
	calculateSpriteLocation(3, 9, 16, 16, 320, 320, &bobX, &bobY);
	DEBUG_LOG("Created RIGHT sprite at (%ld, %ld)\n", bobX, bobY);
	pacman->addSprite(pacman, RIGHT, bobX, bobY, 16, 16);
	calculateSpriteLocation(3, 5, 16, 16, 320, 320, &bobX, &bobY);
	DEBUG_LOG("Created DOWN sprite at (%ld, %ld)\n", bobX, bobY);
	pacman->addSprite(pacman, DOWN, bobX, bobY, 16, 16);
	calculateSpriteLocation(3, 7, 16, 16, 320, 320, &bobX, &bobY);
	DEBUG_LOG("Created LEFT sprite at (%ld, %ld)\n", bobX, bobY);
	pacman->addSprite(pacman, LEFT, bobX, bobY, 16, 16);
	calculateSpriteLocation(3, 11, 16, 16, 320, 320, &bobX, &bobY);
	DEBUG_LOG("Created UP sprite at (%ld, %ld)\n", bobX, bobY);
	pacman->addSprite(pacman, UP, bobX, bobY, 16, 16);
}

void genericCreate(void)
{
	DEBUG_LOG("ACE Main - Starting...\n");

	// Create view with global palette
	g_pView = viewCreate(0, TAG_VIEW_GLOBAL_PALETTE, 1, TAG_DONE);

	// Create viewport
	g_pVPort = vPortCreate(0, TAG_VPORT_VIEW, g_pView, TAG_VPORT_BPP, 5, TAG_DONE);

	// Create simple buffer manager
	g_pBufferManager =
		simpleBufferCreate(0, TAG_SIMPLEBUFFER_VPORT, g_pVPort, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR, TAG_DONE);

	// Load palette from INCBIN data
	paletteDim((UWORD *)colors, g_pVPort->pPalette, 32, 0);

	// Setup pacman tiles bitmap
	setupPacmanTiles();

	// Setup pacman sprite
	setupPacman();

	// Load the view to hardware
	viewLoad(g_pView);

	// Initialize music if enabled
#ifdef MUSIC
	if (p61Init(module) != 0)
		DEBUG_LOG("p61Init failed!\n");
#endif

	// Initialize keyboard manager
	keyCreate();

	// Setup custom VBlank handler
	systemSetInt(INTB_VERTB, (tAceIntHandler)vblankHandler, 0);

	DEBUG_LOG("ACE Main - Ready!\n");
}

void genericProcess(void)
{
	keyProcess();

	if (keyCheck(KEY_ESCAPE))
	{
		DEBUG_LOG("ACE Main - Exiting!\n");
		gameExit();
		return;
	}

	// Handle input
	if (keyCheck(KEY_LEFT) || keyCheck(KEY_A))
		pacman->movePacman(pacman, LEFT);
	else if (keyCheck(KEY_RIGHT) || keyCheck(KEY_D))
		pacman->movePacman(pacman, RIGHT);
	else if (keyCheck(KEY_UP) || keyCheck(KEY_W))
		pacman->movePacman(pacman, UP);
	else if (keyCheck(KEY_DOWN) || keyCheck(KEY_S))
		pacman->movePacman(pacman, DOWN);

	// Get the buffer to draw to
	tBitMap *pBuffer = g_pBufferManager->pBack;

	// Clear previous position
	blitRect(pBuffer, pacman->prevX, pacman->prevY, pacman->width, pacman->height, 0);

	// Draw pacman at new position
	Sprite *currentSprite = pacman->getSprite(pacman, pacman->direction);
	if (currentSprite)
	{
		blitCopyMask(
			tPacmanTiles,
			currentSprite->x, currentSprite->y,
			pBuffer,
			pacman->x, pacman->y,
			pacman->width, pacman->height,
			(const UBYTE *)pacman_tiles_mask);
	}

	// Wait for vblank and swap buffers
	viewProcessManagers(g_pView);
	copProcessBlocks();
	vPortWaitForEnd(g_pVPort);
}

void genericDestroy(void)
{
	DEBUG_LOG("ACE Main - Cleanup...\n");

	// Unload the view first to stop copper DMA
	viewLoad(0);

	// Restore OS control - this is critical!
	systemUse();

	// Clean up keyboard manager
	keyDestroy();

	// Clean up custom interrupt and wait
	systemSetInt(INTB_VERTB, 0, 0);
	WaitVbl();

#ifdef MUSIC
	DEBUG_LOG("End Music!\n");
	p61End();
#endif

	// Free custom resources
	if (tPacmanTiles)
		FreeMem(tPacmanTiles, sizeof(tBitMap));

	// Destroy view (now that it's unloaded)
	viewDestroy(g_pView);

	DEBUG_LOG("ACE Main - Done!\n");
}
