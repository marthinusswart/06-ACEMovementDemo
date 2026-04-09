# ACE Framework Cleanup Process

## Overview

This document outlines the proper initialization and cleanup sequence for ACE framework applications, based on issues encountered during development and solutions found in production ACE games.

## The Problem

When exiting the game back to AmigaDOS, the floppy drive would continue making seeking sounds, indicating that system resources weren't being properly restored. Additionally, keyboard input wasn't being detected during gameplay.

## Root Causes

Two critical issues were identified:

1. **Missing `keyCreate()` initialization** - The keyboard manager was never initialized, so keypresses weren't being captured
2. **Missing `systemUse()` in cleanup** - OS control wasn't being restored, leaving hardware (especially disk DMA) in an undefined state

## Required Initialization

The keyboard manager must be explicitly initialized before it can detect keypresses:

```c
void genericCreate(void)
{
    // ... other initialization ...

    // Initialize keyboard manager
    keyCreate();

    // ... rest of setup ...
}
```

**Why this is necessary:**

- `keyCreate()` sets up the CIA interrupt handler for keyboard input
- Without it, `keyProcess()` and `keyCheck()` won't work because no keys are being captured at the hardware level
- The interrupt handler is what translates raw keyboard matrix scans into key codes

## Proper Cleanup Sequence

Based on analysis of production ACE games ([germz](https://github.com/Last-Minute-Creations/germz) and [AMIner](https://github.com/Last-Minute-Creations/AMIner)), the correct cleanup sequence is:

### 1. Unload the View

```c
viewLoad(0);
```

This stops the copper list from running and prevents further DMA operations.

### 2. Restore OS Control

```c
systemUse();
```

**This is the critical step** that restores OS control over:

- Disk DMA (stops floppy drive activity)
- Interrupts
- Blitter
- Other system resources

### 3. Clean Up Managers

```c
keyDestroy();
systemSetInt(INTB_VERTB, 0, 0);
WaitVbl();
```

Clean up custom managers and interrupts in reverse order of initialization.

### 4. Clean Up Audio/Music

```c
#ifdef MUSIC
p61End();
#endif
```

### 5. Free Custom Resources

```c
if (tPacmanTiles)
    FreeMem(tPacmanTiles, sizeof(tBitMap));
```

### 6. Destroy View

```c
viewDestroy(g_pView);
```

This automatically handles viewport and buffer cleanup through the ACE framework's hierarchical destruction.

## Complete Example

```c
void genericCreate(void)
{
    KPrintF("ACE Main - Starting...\n");

    // Create view with global palette
    g_pView = viewCreate(0, TAG_VIEW_GLOBAL_PALETTE, 1, TAG_DONE);

    // Create viewport
    g_pVPort = vPortCreate(0, TAG_VPORT_VIEW, g_pView, TAG_VPORT_BPP, 5, TAG_DONE);

    // Create simple buffer manager
    g_pBufferManager =
        simpleBufferCreate(0, TAG_SIMPLEBUFFER_VPORT, g_pVPort, TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR, TAG_DONE);

    // Load palette from INCBIN data
    paletteDim((UWORD *)colors, g_pVPort->pPalette, 32, 0);

    // Setup custom resources
    setupPacmanTiles();
    setupPacman();

    // Load the view to hardware
    viewLoad(g_pView);

    // Initialize music if enabled
#ifdef MUSIC
    if (p61Init(module) != 0)
        KPrintF("p61Init failed!\n");
#endif

    // Initialize keyboard manager - CRITICAL!
    keyCreate();

    // Setup custom VBlank handler
    systemSetInt(INTB_VERTB, (tAceIntHandler)vblankHandler, 0);

    KPrintF("ACE Main - Ready!\n");
}

void genericDestroy(void)
{
    KPrintF("ACE Main - Cleanup...\n");

    // 1. Unload the view first to stop copper DMA
    viewLoad(0);

    // 2. Restore OS control - CRITICAL for proper system restoration!
    systemUse();

    // 3. Clean up keyboard manager
    keyDestroy();

    // 4. Clean up custom interrupt and wait
    systemSetInt(INTB_VERTB, 0, 0);
    WaitVbl();

#ifdef MUSIC
    // 5. Stop music
    KPrintF("End Music!\n");
    p61End();
#endif

    // 6. Free custom resources
    if (tPacmanTiles)
        FreeMem(tPacmanTiles, sizeof(tBitMap));

    // 7. Destroy view (now that it's unloaded)
    viewDestroy(g_pView);

    KPrintF("ACE Main - Done!\n");
}
```

## Examples from Production ACE Games

### germz - gameGsDestroy()

From [germz/src/game.c](https://github.com/Last-Minute-Creations/germz/blob/master/src/game.c):

```c
static void gameGsDestroy(void) {
    viewLoad(0);
    systemUse();        // ← Restores OS control

    assetsGameDestroy();
    playerDestroy();
    bobNewManagerDestroy();
    aiDestroy();
    fadeDestroy(s_pFade);

    viewDestroy(s_pView);
}
```

### AMIner - genericDestroy()

From [AMIner/src/aminer.c](https://github.com/Last-Minute-Creations/AMIner/blob/master/src/aminer.c):

```c
void genericDestroy(void) {
    fontDestroy(g_pFont);
    ptplayerDestroy();

#if defined(USE_PAK_FILE)
    pakFileClose(g_pPakFile);
#endif

    stateManagerDestroy(g_pGameStateManager);
    keyDestroy();
    joyClose();
}
```

**Note:** AMIner's view management happens within its game states, not in the main genericDestroy. The pattern of cleaning up managers in reverse initialization order is consistent.

## Key Principles

1. **Hierarchical Cleanup**: `viewDestroy()` automatically destroys attached viewports and their buffer managers
2. **Reverse Initialization Order**: Clean up resources in the opposite order they were created
3. **System Restoration**: Always call `systemUse()` after `viewLoad(0)` to restore OS control
4. **Wait for Hardware**: Use `WaitVbl()` after clearing interrupts to ensure hardware operations complete
5. **Manager Pairing**: Every `xxxCreate()` must have a corresponding `xxxDestroy()`

## Common Mistakes

### ❌ Missing keyCreate()

```c
void genericCreate(void) {
    // ... setup ...
    // Missing: keyCreate();
}

void genericProcess(void) {
    keyProcess();  // Won't work - keyboard never initialized!
    if (keyCheck(KEY_ESCAPE)) { ... }
}
```

### ❌ Missing systemUse()

```c
void genericDestroy(void) {
    viewLoad(0);
    // Missing: systemUse();  ← Floppy will keep grinding!
    viewDestroy(g_pView);
}
```

### ❌ Wrong Cleanup Order

```c
void genericDestroy(void) {
    viewDestroy(g_pView);  // Wrong - destroy before unloading!
    viewLoad(0);
}
```

## Framework Cleanup Sequence

The ACE framework's `main()` function (from `ace/generic/main.h`) handles the outer cleanup:

```c
int main(void) {
    systemCreate();
    logOpen(GENERIC_MAIN_LOG_PATH);
    memCreate();
    timerCreate();
    blitManagerCreate();
    copCreate();

    genericCreate();        // Your initialization
    while (GENERIC_MAIN_LOOP_CONDITION) {
        timerProcess();
        genericProcess();   // Your game loop
    }
    genericDestroy();       // Your cleanup

    copDestroy();
    blitManagerDestroy();
    timerDestroy();
    memDestroy();
    logClose();
    systemDestroy();        // Final system restoration

    return EXIT_SUCCESS;
}
```

Your `genericDestroy()` is called **before** the framework's own cleanup, which handles final system restoration via `systemDestroy()`.

## System State Management

### systemUnuse() / systemUse() Pairing

The ACE framework uses a reference counting system for OS suspension:

- **systemUnuse()** - Suspends the OS, takes control of hardware
  - Disables OS interrupts and DMA
  - Allows your game full hardware access
  - Used internally by ACE for game operations

- **systemUse()** - Restores OS control temporarily
  - Re-enables OS interrupts and DMA
  - Used for file I/O, disk operations
  - Must be called in cleanup to restore system state

**Important:** You typically don't call `systemUnuse()` yourself - the ACE framework handles this. But you **must** call `systemUse()` in your cleanup to restore OS control before the framework's final `systemDestroy()`.

## References

### ACE Framework

- [ACE GitHub Repository](https://github.com/AmigaPorts/ACE)
- [ACE Documentation](https://github.com/AmigaPorts/ACE/blob/main/README.md)

### Example Games

- [germz](https://github.com/Last-Minute-Creations/germz) - Clean example of view management and cleanup
- [AMIner](https://github.com/Last-Minute-Creations/AMIner) - State-based game architecture with ACE
- [OpenFire](https://github.com/Last-Minute-Creations/openFire) - Another production ACE game
- [Chaos Arena](https://github.com/Last-Minute-Creations/chaosArena) - Multiplayer game using ACE

### Framework Source Files

- `framework/ace/src/ace/managers/key.c` - Keyboard manager implementation
- `framework/ace/src/ace/managers/system.c` - System management (systemUse/systemUnuse)
- `framework/ace/src/ace/utils/extview.c` - View/viewport management
- `framework/ace/include/ace/generic/main.h` - Main loop and framework structure

## Troubleshooting

### Floppy Drive Activity After Exit

**Symptom:** Floppy drive seeks/grinds after returning to AmigaDOS  
**Cause:** Missing `systemUse()` call in cleanup  
**Solution:** Add `systemUse()` immediately after `viewLoad(0)`

### Keyboard Not Working

**Symptom:** `keyCheck()` always returns false  
**Cause:** Missing `keyCreate()` in initialization  
**Solution:** Add `keyCreate()` in `genericCreate()` and `keyDestroy()` in `genericDestroy()`

### Crashes on Exit

**Symptom:** Guru meditation or system freeze on exit  
**Cause:** Cleaning up resources in wrong order or using freed memory  
**Solution:** Follow the cleanup sequence: viewLoad(0) → systemUse() → cleanup managers → viewDestroy()

### Screen Artifacts After Exit

**Symptom:** Corrupted display or wrong colors after exit  
**Cause:** Copper list still running or DMA not stopped  
**Solution:** Ensure `viewLoad(0)` is called first in cleanup

## Version History

- **2026-04-08** - Initial documentation based on development findings and analysis of germz and AMIner source code
