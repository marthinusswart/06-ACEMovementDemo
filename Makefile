# to generate assembler listing with LTO, add to LDFLAGS: -Wa,-adhln=$@.listing,--listing-rhs-width=200
# for better annotations add -dA -dP
# to generate assembler source with LTO, add to LDFLAGS: -save-temps=cwd

ifdef OS
	WINDOWS = 1
	SHELL = cmd.exe
endif

# Recursive wildcard function for deep directories (like the ACE framework)
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(if $(wildcard $d/),$(call rwildcard,$d/,$2)))

subdirs := $(wildcard */)
VPATH = $(subdirs)
cpp_sources := $(wildcard *.cpp) $(wildcard $(addsuffix *.cpp,$(subdirs)))
c_sources := $(wildcard *.c) $(wildcard $(addsuffix *.c,$(subdirs))) $(call rwildcard,framework/ace/src/,*.c)
s_sources := support/gcc8_a_support.s support/depacker_doynax.s $(call rwildcard,framework/ace/src/,*.s)
vasm_sources := $(wildcard *.asm) $(wildcard $(addsuffix *.asm, $(subdirs)))

# Automatically set VPATH to all directories containing our discovered source files
VPATH = $(sort $(dir $(cpp_sources) $(c_sources) $(s_sources) $(vasm_sources)))

cpp_objects := $(sort $(addprefix obj/,$(patsubst %.cpp,%.o,$(notdir $(cpp_sources)))))
c_objects := $(sort $(addprefix obj/,$(patsubst %.c,%.o,$(notdir $(c_sources)))))
s_objects := $(sort $(addprefix obj/,$(patsubst %.s,%.o,$(notdir $(s_sources)))))
vasm_objects := $(sort $(addprefix obj/,$(patsubst %.asm,%.o,$(notdir $(vasm_sources)))))
objects := $(cpp_objects) $(c_objects) $(s_objects) $(vasm_objects)

# https://stackoverflow.com/questions/4036191/sources-from-subdirectories-in-makefile/4038459
# http://www.microhowto.info/howto/automatically_generate_makefile_dependencies.html

program = out/a
OUT = $(program)
CC = m68k-amiga-elf-gcc
AS = m68k-amiga-elf-as
VASM = vasmm68k_mot

ifdef WINDOWS
	SDKDIR = $(abspath $(dir $(shell where $(CC)))..\m68k-amiga-elf\sys-include)
	MKDIR = mkdir "$(@D)" 2>nul
else
	SDKDIR = $(abspath $(dir $(shell which $(CC)))../m68k-amiga-elf/sys-include)
	MKDIR = mkdir -p "$(@D)"
endif

CCFLAGS   = -g -MP -MMD -m68000 -Ofast -nostdlib -Wextra -Wno-unused-function -Wno-volatile-register-var -fomit-frame-pointer -fno-tree-loop-distribution -flto -fwhole-program -fno-exceptions -ffunction-sections -fdata-sections -I. -Iframework/ace/include -Iframework/ace/include/mini_std -DAMIGA -DBARTMAN_GCC -DACE_SCROLLBUFFER_X_MARGIN_SIZE=1 -DACE_SCROLLBUFFER_Y_MARGIN_SIZE=1 -DVSCODE -DACE_TILEBUFFER_TILE_TYPE=UWORD
CPPFLAGS  = $(CCFLAGS) -fno-rtti -fcoroutines -fno-use-cxa-atexit 
ASFLAGS   = -mcpu=68000 -g --register-prefix-optional -I$(SDKDIR)
LDFLAGS   = -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map
VASMFLAGS = -m68000 -Felf -opt-fconst -nowarn=62 -dwarf=3 -quiet -x -I. -I$(SDKDIR)

all: $(OUT).exe

$(OUT).exe: $(OUT).elf
	$(info Elf2Hunk $(program).exe)
	@elf2hunk $(OUT).elf $(OUT).exe

$(OUT).elf: $(objects)
	$(info Linking $(program).elf)
	@$(CC) $(CCFLAGS) $(LDFLAGS) $(objects) -o $@
	@m68k-amiga-elf-objdump --disassemble --no-show-raw-ins --visualize-jumps -S $@ >$(OUT).s

clean:
	$(info Cleaning...)
ifdef WINDOWS
	@-rmdir /s /q obj 2>nul
	@-rmdir /s /q out 2>nul
else
	@$(RM) -r obj out
endif

-include $(objects:.o=.d)

$(cpp_objects) : obj/%.o : %.cpp
	$(info Compiling $<)
	@-$(MKDIR)
	@$(CC) $(CPPFLAGS) -c -o $@ $(CURDIR)/$<

$(c_objects) : obj/%.o : %.c
	$(info Compiling $<)
	@-$(MKDIR)
	@$(CC) $(CCFLAGS) -c -o $@ $(CURDIR)/$<

$(s_objects): obj/%.o : %.s
	$(info Assembling $<)
	@-$(MKDIR)
	@$(AS) $(ASFLAGS) --MD $(@D)/$*.d -o $@ $(CURDIR)/$<

$(vasm_objects): obj/%.o : %.asm
	$(info Assembling $<)
	@-$(MKDIR)
	@$(VASM) $(VASMFLAGS) -dependall=make -depfile $(@D)/$*.d -o $@ $(CURDIR)/$<
