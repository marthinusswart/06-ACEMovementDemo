#include "SystemRoutines.h"
#include "ScreenRoutines.h"
#include <proto/exec.h>
#include <proto/graphics.h>
#include <graphics/gfxbase.h>
#include <graphics/view.h>
#include <hardware/custom.h>

extern volatile struct Custom *custom;
extern struct GfxBase *GfxBase;
extern struct View *ActiView;

extern UWORD SystemInts;
extern UWORD SystemDMA;
extern UWORD SystemADKCON;
extern APTR SystemIrq;
extern volatile APTR VBR;

void TakeSystem(void)
{
    Forbid();
    // Save current interrupts and DMA settings so we can restore them upon exit.
    SystemADKCON = custom->adkconr;
    SystemInts = custom->intenar;
    SystemDMA = custom->dmaconr;
    ActiView = GfxBase->ActiView; // store current view

    LoadView(0);
    WaitTOF();
    WaitTOF();

    WaitVbl();
    WaitVbl();

    OwnBlitter();
    WaitBlt();
    Disable();

    custom->intena = 0x7fff; // disable all interrupts
    custom->intreq = 0x7fff; // Clear any interrupts that were pending

    custom->dmacon = 0x7fff; // Clear all DMA channels

    // set all colors black
    for (int a = 0; a < 32; a++)
        custom->color[a] = 0;

    WaitVbl();
    WaitVbl();

    VBR = GetVBR();
    SystemIrq = GetInterruptHandler(); // store interrupt register
}

void FreeSystem(void)
{
    WaitVbl();
    WaitBlt();
    custom->intena = 0x7fff; // disable all interrupts
    custom->intreq = 0x7fff; // Clear any interrupts that were pending
    custom->dmacon = 0x7fff; // Clear all DMA channels

    // restore interrupts
    SetInterruptHandler(SystemIrq);

    /*Restore system copper list(s). */
    custom->cop1lc = (ULONG)GfxBase->copinit;
    custom->cop2lc = (ULONG)GfxBase->LOFlist;
    custom->copjmp1 = 0x7fff; // start coppper

    /*Restore all interrupts and DMA settings. */
    custom->intena = SystemInts | 0x8000;
    custom->dmacon = SystemDMA | 0x8000;
    custom->adkcon = SystemADKCON | 0x8000;

    WaitBlt();
    DisownBlitter();
    Enable();

    LoadView(ActiView);
    WaitTOF();
    WaitTOF();

    Permit();
}