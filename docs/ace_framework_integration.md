# Integrating ACE Framework with VSCode Amiga C/C++ Compile Extension

This document details the modifications required to successfully compile and link the Amiga C Engine (ACE) framework within a project using the **VSCode Amiga C/C++ Compile, Debug & Profile** extension (by Bartman/Abyss).

Because the ACE framework relies on CMake by default, pulling its source directly into a custom GNU Makefile and a flattened object directory requires several build-system and source-level adjustments.

---

## 1. Makefile Modifications

To compile the ACE framework alongside the main project code, the `makefile` was updated to discover the deep directory structure of ACE and pass the correct configuration macros to the compiler.

### Recursive Source Discovery

The standard `$(wildcard */)` command only searches one directory deep. A recursive wildcard macro was added to crawl through the `framework/ace/src/` folder:

```makefile
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(if $(wildcard $d/),$(call rwildcard,$d/,$2)))
```

This allows variables like `c_sources` to automatically find every `.c` file regardless of how deep it is nested in the ACE framework.

### `VPATH` and Target Deduplication

Because all `.o` files are dumped into a flat `obj/` directory, `VPATH` was updated to map to all discovered source directories.
Additionally, `$(sort ...)` was wrapped around the object lists to inherently deduplicate identical filenames (e.g., `sprite.o` and `string.o`), suppressing Make warnings about multiple rules for the same target:

```makefile
VPATH = $(sort $(dir $(cpp_sources) $(c_sources) $(s_sources) $(vasm_sources)))
c_objects := $(sort $(addprefix obj/,$(patsubst %.c,%.o,$(notdir $(c_sources)))))
```

### Compiler Flags & Defines (`CCFLAGS`)

Several paths and defines were added to `CCFLAGS` to satisfy ACE's configuration requirements:

- **Include Paths:**
  - `-Iframework/ace/include`: Resolves standard ACE framework headers.
  - `-Iframework/ace/include/mini_std`: Resolves missing lightweight standard headers (e.g., `stdint.h`) since `-nostdlib` is used.
- **Environment Defines:**
  - `-DAMIGA`: Enables Amiga-specific logic and OS includes inside the ACE headers.
  - `-DBARTMAN_GCC`: Tells ACE which specific Amiga toolchain is being used so it can map its idle hooks properly.
  - `-DVSCODE`: A custom project flag used to distinguish our local setup from the default Bartman CMake structure.
- **ACE Configuration Defines:**
  - `-DACE_SCROLLBUFFER_X_MARGIN_SIZE=1` & `-DACE_SCROLLBUFFER_Y_MARGIN_SIZE=1`: Configures the off-screen tile margin for the scroll buffer manager.
  - `-DACE_TILEBUFFER_TILE_TYPE=UWORD`: Configures tilemaps to use 16-bit indices (allowing up to 65,535 unique tiles) instead of the default 8-bit limit.

---

## 2. Source File Adjustments

A few minor tweaks were made to the ACE framework source files and local support files to resolve missing types and linker collisions caused by the integration.

### `framework/ace/include/ace/managers/blit.h`

- **Issue:** The compiler threw errors about undeclared identifiers like `ABC`, `SRCA`, and `DEST`.
- **Fix:** Added `#include <hardware/blitter.h>` inside the `#ifdef AMIGA` block to pull in the Amiga hardware constants for blitter line modes.

### `framework/ace/src/ace/managers/advancedsprite.c`

- **Issue:** The compiler threw errors about unknown type names `tCopList` and `tCopBlock`.
- **Fix:** Added `#include <ace/managers/copper.h>` so the compiler knows about the copper structures before they are referenced by the sprite utility header.

### `support/gcc8_c_support.h`

- **Issue:** The compiler issued a warning that `offsetof` was being redefined.
- **Fix:** Wrapped the local macro definition in an `#ifndef offsetof` block so it defers to the standard definition if it was already included via ACE's standard headers:

```c
#ifndef offsetof
#define offsetof(st, m) __builtin_offsetof(st, m)
#endif
```
