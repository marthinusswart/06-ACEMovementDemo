# ACE Framework Upgrade Guide

If you pull the latest ACE framework code from GitHub and overwrite the `framework/ace/` folder, you will lose the source-level adjustments made to ensure compatibility with the VSCode Bartman/Abyss toolchain.

Follow these steps to re-apply the necessary fixes to the fresh ACE files:

## Step 1: Overwrite the Framework

Copy the latest ACE source code into `framework/ace/`, replacing the existing files. (Your root `makefile` and `support/` folder are safe as they live outside this directory).

## Step 2: Fix the Blitter Header

Open `framework/ace/include/ace/managers/blit.h`.
Find the `#ifdef AMIGA` block at the top and add `<hardware/blitter.h>`:

```c
#ifdef AMIGA
#include <exec/interrupts.h>  // struct Interrupt
#include <hardware/dmabits.h> // DMAF_BLITTER
#include <hardware/intbits.h> // INTB_BLIT
#include <hardware/blitter.h> // ABC, SRCA, DEST constants
#endif // AMIGA
```

## Step 3: Fix the Advanced Sprite Manager

Open `framework/ace/src/ace/managers/advancedsprite.c`.
Add `#include <ace/managers/copper.h>` to the includes at the top:

```c
#include <ace/managers/advancedsprite.h>
#include <ace/utils/bitmap.h>
#include <ace/managers/blit.h>
#include <ace/managers/copper.h>
#include <ace/utils/sprite.h>
```

## Step 4: Fix the System Manager

Open `framework/ace/src/ace/managers/system.c`.
Search for `<bartman/gcc8_c_support.h>` and replace it with:

```c
#if defined(BARTMAN_GCC)
#if defined(VSCODE)
#include "support/gcc8_c_support.h" // Idle measurement
#else
#include <bartman/gcc8_c_support.h> // Idle measurement
#endif
#endif
```

Then, find where `GfxBase`, `DOSBase`, and `SysBase` are declared (usually around lines 125-145) and wrap them so they are skipped when `VSCODE` is defined:

```c
#if !defined(VSCODE)
struct GfxBase *GfxBase = 0;
#endif
// ... further down ...
#if defined(BARTMAN_GCC)
#if !defined(VSCODE)
struct DosLibrary *DOSBase = 0;
struct ExecBase *SysBase = 0;
#endif
```

## Step 5: Rebuild

Run `make clean` and `make` to compile the fresh framework code!
