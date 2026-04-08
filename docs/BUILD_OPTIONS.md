# Build Options

This project supports two different implementations:

## 1. Manual Implementation (main.c) - DEFAULT
The original close-to-the-metal approach that manually sets up:
- Custom copper lists
- Direct hardware register manipulation
- Manual screen and sprite setup

**To build:**
```bash
make
```

## 2. ACE Framework Implementation (ace_main.c)
A cleaner implementation using the ACE framework's high-level managers:
- View/Viewport system
- Simple buffer manager
- Automatic double buffering
- Cleaner abstraction over hardware

**To build:**
```bash
make USE_ACE_MAIN=1
```

## Switching Between Versions

To switch between implementations, clean and rebuild:

```bash
# Build with manual implementation
make clean
make

# Build with ACE implementation
make clean
make USE_ACE_MAIN=1
```

Both versions provide the same Pacman movement demo functionality, just with different underlying implementations.
