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

// DEMO - INCBIN
volatile short frameCounter = 0;

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

static __attribute__((interrupt)) void interruptHandler()
{
	custom->intreq = (1 << INTB_VERTB);
	custom->intreq = (1 << INTB_VERTB); // reset vbl req. twice for a4000 bug.

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

int main()
{
	SysBase = *((struct ExecBase **)4UL);
	custom = (struct Custom *)0xdff000;

	// We will use the graphics library only to locate and restore the system copper list once we are through.
	GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR) "graphics.library", 0);
	if (!GfxBase)
		Exit(0);

	// used for printing
	DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR) "dos.library", 0);
	if (!DOSBase)
		Exit(0);

#ifdef __cplusplus
	KPrintF("Hello debugger from Amiga: %ld!\n", staticClass.i);
#else
	KPrintF("Hello debugger from Amiga!\n");
#endif
	Write(Output(), (APTR) "Hello console!\n", 15);
	Delay(50);

	warpmode(1);
	// TODO: precalc stuff here
#ifdef MUSIC
	if (p61Init(module) != 0)
		KPrintF("p61Init failed!\n");
#endif
	warpmode(0);

	TakeSystem();
	WaitVbl();

	USHORT *copper1 = (USHORT *)AllocMem(1024, MEMF_CHIP);
	USHORT *copPtr = copper1;

	copPtr = screenScanDefault(copPtr);
	// setup bitplane control (0 bitplanes since we just want the background color)
	*copPtr++ = offsetof(struct Custom, bplcon0);
	*copPtr++ = (0 << 10) /*dual pf*/ | (1 << 9) /*color*/ | ((0) << 12) /*num bitplanes*/;

	// jump to copper2
	*copPtr++ = offsetof(struct Custom, copjmp2);
	*copPtr++ = 0x7fff;

	custom->cop1lc = (ULONG)copper1;
	custom->cop2lc = (ULONG)copper2;
	custom->dmacon = DMAF_BLITTER; // disable blitter dma for copjmp bug
	custom->copjmp1 = 0x7fff;	   // start coppper
	custom->dmacon = DMAF_SETCLR | DMAF_MASTER | DMAF_RASTER | DMAF_COPPER | DMAF_BLITTER;

	// DEMO
	SetInterruptHandler((APTR)interruptHandler);
	custom->intena = INTF_SETCLR | INTF_INTEN | INTF_VERTB;
#ifdef MUSIC
	custom->intena = INTF_SETCLR | INTF_EXTER; // ThePlayer needs INTF_EXTER
#endif

	custom->intreq = (1 << INTB_VERTB); // reset vbl req

	while (!MouseLeft())
	{
		Wait10();
	}

#ifdef MUSIC
	p61End();
#endif

	// END
	FreeSystem();

	CloseLibrary((struct Library *)DOSBase);
	CloseLibrary((struct Library *)GfxBase);
}
