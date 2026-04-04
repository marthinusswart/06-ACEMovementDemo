#include "support/gcc8_c_support.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <exec/execbase.h>
#include <graphics/gfxmacros.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "routines/ScreenRoutines.h"
#include "routines/SystemRoutines.h"
#include "routines/MouseRoutines.h"
#include "routines/MusicRoutines.h"
#include "routines/CopperRoutines.h"
#include <ace/managers/system.h>
#include <ace/managers/key.h>

// config
#define MUSIC

struct ExecBase *SysBase;
volatile struct Custom *custom;
struct DosLibrary *DOSBase;
struct GfxBase *GfxBase;

// backup
UWORD SystemInts;
UWORD SystemDMA;
UWORD SystemADKCON;
volatile APTR VBR = 0;
APTR SystemIrq;

struct View *ActiView;

#define BG_SIZE ((320 / 8) * 256 * 5) // 51,200 bytes
UBYTE *screen_buffer = NULL;		  // Active display buffer

// DEMO - INCBIN
volatile short frameCounter = 0;
INCBIN(colors, "pal/pacman_tiles.pal")
INCBIN_CHIP(pacman_tiles, "bpl/pacman_tiles.bpl")

// put copperlist into chip mem so we can use it without copying
const UWORD copper2[] __attribute__((section(".MEMF_CHIP"))) = {
	offsetof(struct Custom, color[0]), 0x0000,
	0x4101, 0xff00, offsetof(struct Custom, color[0]), 0x0111, // line 0x41
	0x4201, 0xff00, offsetof(struct Custom, color[0]), 0x0222, // line 0x42
	0x4301, 0xff00, offsetof(struct Custom, color[0]), 0x0333, // line 0x43
	0x4401, 0xff00, offsetof(struct Custom, color[0]), 0x0444, // line 0x44
	0x4501, 0xff00, offsetof(struct Custom, color[0]), 0x0555, // line 0x45
	0x4601, 0xff00, offsetof(struct Custom, color[0]), 0x0666, // line 0x46
	0x4701, 0xff00, offsetof(struct Custom, color[0]), 0x0777, // line 0x47
	0x4801, 0xff00, offsetof(struct Custom, color[0]), 0x0888, // line 0x48
	0x4901, 0xff00, offsetof(struct Custom, color[0]), 0x0999, // line 0x49
	0x4a01, 0xff00, offsetof(struct Custom, color[0]), 0x0aaa, // line 0x4a
	0x4b01, 0xff00, offsetof(struct Custom, color[0]), 0x0bbb, // line 0x4b
	0x4c01, 0xff00, offsetof(struct Custom, color[0]), 0x0ccc, // line 0x4c
	0x4d01, 0xff00, offsetof(struct Custom, color[0]), 0x0ddd, // line 0x4d
	0x4e01, 0xff00, offsetof(struct Custom, color[0]), 0x0eee, // line 0x4e
	0x4f01, 0xff00, offsetof(struct Custom, color[0]), 0x0fff, // line 0x4e
	0xffff, 0xfffe											   // end copper list
};

static void vblankHandler(volatile struct Custom *pCustom, volatile void *pData)
{
#ifdef MUSIC
	// DEMO - ThePlayer
	p61Music();
#endif
	// DEMO - increment frameCounter
	frameCounter++;
}

#ifdef __cplusplus
class TestClass
{
public:
	TestClass(int y)
	{
		static int x = 7;
		i = y + x;
	}
	~TestClass()
	{
		KPrintF("~TestClass()");
	}

	int i;
};

TestClass staticClass(4);
#endif

static void Wait10() { WaitLine(0x10); }
static void Wait11() { WaitLine(0x11); }
static void Wait12() { WaitLine(0x12); }
static void Wait13() { WaitLine(0x13); }

static void setupEnvironment(void)
{
	SysBase = *((struct ExecBase **)4UL);
	custom = (struct Custom *)0xdff000;

	DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR) "dos.library", 0);
	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR) "graphics.library", 0);

#ifdef __cplusplus
	KPrintF("Hello debugger from Amiga: %ld!\n", staticClass.i);
#else
	KPrintF("Hello debugger from Amiga!\n");
#endif

	if (DOSBase)
	{
		Write(Output(), (APTR) "Hello console!\n", 15);
		Delay(50); // This requires the OS timer.device to be alive!
	}

	// ACE's system.c comments claim to save ADKCON, but actually forgets to!
	// We must back it up manually to ensure disk drives and audio return to normal.
	SystemADKCON = custom->adkconr;
	SystemInts = custom->intenar;
	SystemDMA = custom->dmaconr;

	systemCreate(); // Seize the system BEFORE p61Init pollutes the hardware registers
	systemUnuse();	// Fully suspend the OS so it stops overwriting the copper list

	warpmode(1);
}

static void teardownEnvironment(void)
{
	KPrintF("Destroying System!\n");
	systemDestroy();

	// MANUALLY RESTORE STATE (Ignoring whatever ACE missed/messed up)
	// ACE uses LoadView() which waits for the OS, but the OS might be too slow
	// causing our FreeMem to rip the Copper list out from under the hardware!
	Disable();
	custom->intena = 0x7FFF; // Disable all
	custom->intreq = 0x7FFF; // Clear pending
	custom->dmacon = 0x7FFF; // Disable all DMA

	// Forcefully point the hardware back to the OS Copper lists INSTANTLY
	custom->cop1lc = (ULONG)GfxBase->copinit;
	custom->cop2lc = (ULONG)GfxBase->LOFlist;
	custom->copjmp1 = 0x7fff;

	custom->intena = SystemInts | 0x8000;
	custom->dmacon = SystemDMA | 0x8000;
	custom->adkcon = SystemADKCON | 0x8000;
	Enable();

	if (GfxBase)
	{
		KPrintF("free gfx library!\n");
		CloseLibrary((struct Library *)GfxBase);
	}

	if (DOSBase)
	{
		KPrintF("free dos library!\n");
		CloseLibrary((struct Library *)DOSBase);
	}
}

static USHORT *setupCopper(void)
{
	USHORT *copper1 = (USHORT *)AllocMem(1024, MEMF_CHIP);
	USHORT *copPtr = copper1;
	// Allocate the active screen buffer and copy the clean background into it
	screen_buffer = (UBYTE *)AllocMem(BG_SIZE, MEMF_CHIP | MEMF_CLEAR); // Clear memory so it doesn't show garbage

	copPtr = screenScanDefault(copPtr);

	// enable bitplanes
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0 << 10) /*dual pf*/ | (1 << 9) /*color*/ | ((5) << 12) /*num bitplanes*/;
	*copPtr++ = offsetof(struct Custom, bplcon1); // scrolling
	*copPtr++ = 0;
	*copPtr++ = offsetof(struct Custom, bplcon2); // playfied priority
	*copPtr++ = 1 << 6;							  // 0x24;			//Sprites have priority over playfields

	const USHORT lineSize = 320 / 8;

	// set bitplane modulo
	*copPtr++ = offsetof(struct Custom, bpl1mod); // odd planes   1,3,5
	*copPtr++ = 4 * lineSize;
	*copPtr++ = offsetof(struct Custom, bpl2mod); // even  planes 2,4
	*copPtr++ = 4 * lineSize;

	// set bitplane pointers
	const UBYTE *planes[5];
	for (int a = 0; a < 5; a++)
		planes[a] = screen_buffer + lineSize * a;
	copPtr = copSetPlanes(0, copPtr, planes, 5); // INJECT pointers into copper list!

	// set colors
	for (int a = 0; a < 32; a++)
		copPtr = copSetColor(copPtr, a, ((USHORT *)colors)[a]);

	// jump to copper2
	*copPtr++ = offsetof(struct Custom, copjmp2);
	*copPtr++ = 0x7fff;

	custom->cop1lc = (ULONG)copper1;
	custom->cop2lc = (ULONG)copper2;
	custom->dmacon = DMAF_BLITTER; // disable blitter dma for copjmp bug
	custom->copjmp1 = 0x7fff;	   // start coppper

	return copper1;
}

int main()
{
	setupEnvironment();
	// TODO: precalc stuff here
#ifdef MUSIC
	if (p61Init(module) != 0)
		KPrintF("p61Init failed!\n");
#endif
	warpmode(0);
	WaitVbl();

	USHORT *copper1 = setupCopper();

	systemSetDmaMask(DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER, 1); // Tell ACE to enable DMA

	// DEMO
	systemSetInt(INTB_VERTB, vblankHandler, 0);
	keyCreate();

	while (!MouseLeft())
	{
		WaitVbl();
		if (keyCheck(KEY_ESCAPE))
			break;
		keyProcess(); // Process pending keystrokes from the CIA interrupt buffer
	}
	KPrintF("Exit Loop!\n");
	keyDestroy();

	// Clean up custom interrupt before waking up the OS
	// Otherwise the hardware will call it after the program has exited!
	KPrintF("Reset Interrupt!\n");
	systemSetInt(INTB_VERTB, 0, 0);
	WaitVbl(); // Wait 1 frame to guarantee the handler is fully finished

	KPrintF("Free Copper List!\n");
	FreeMem(copper1, 1024);

#ifdef MUSIC
	KPrintF("End Music!\n");
	p61End(); // End the music player safely BEFORE waking up the OS!
#endif

	// END
	teardownEnvironment();
}
