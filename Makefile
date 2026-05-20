###############################################################################
# File: Makefile
# Description: CDFJ+ Template Makefile - staged/portable build
# (C) Copyright 2017 - Chris Walton, Fred Quimby, Darrell Spice Jr
# Additions by Craig Daniels - Gamax Software - 2026
# updated by Andrew Davie May/2026
###############################################################################


###############################################################################
# Instance-unique paths -- where are things located on your system?

TOOLCHAIN 	= ~/Documents/software/gcc-arm-none-eabi/bin/arm-none-eabi
DASM 		= ~/Documents/software/Atari\ 2600/dasm/bin/dasm
GOPHER 		= ~/Documents/software/Atari\ 2600/Gopher2600/
GOPHERNAME  = gopher2600_darwin_arm64
STELLA 		= /Applications/Stella.app

# Note: on MacOS Intel build of Gopher will likely be suffixed with _amd64
###############################################################################


.NOTPARALLEL:

# Tool names
CC        = $(TOOLCHAIN)-gcc
AS        = $(TOOLCHAIN)-as
LD        = $(TOOLCHAIN)-ld
OBJCOPY   = $(TOOLCHAIN)-objcopy
SIZE      = $(TOOLCHAIN)-size




# Dirs/files
SOURCE = cdfj+_template
BASE   = main
CUSTOM    = $(BASE)/custom
BIN    = $(BASE)/bin
OUTPUT = output

DASM_TO_C = defines_dasm.h

CFLAGS = -g3 -gdwarf-4 -gstrict-dwarf -mcpu=arm7tdmi -march=armv4t -mthumb \
         -Wall -Wextra -Wno-unused-function -ffunction-sections -Os \
        -Wl,--print-memory-usage,--build-id=none -flto -fno-builtin \
		-mno-thumb-interwork -fextended-identifiers \
        -MMD -MP

# ARM/C output
CUSTOMNAME    = WenHopPlanetPluto
CDFJ = cdfj+
CUSTOMELF     = $(BIN)/$(CDFJ).elf
CUSTOMBIN     = $(BIN)/$(CDFJ).bin
CUSTOMMAP     = $(BIN)/$(CDFJ).map
CUSTOMLST     = $(BIN)/$(CDFJ).lst
CUSTOMLINK    = $(CUSTOM)/custom.boot.lds
CUSTOMTARGETS = $(CUSTOMELF) $(CUSTOMBIN)
CUSTOMOBJS    = ASM_routines.o

###############################################################################
# append new .c files to SRCS

SRCS = \
 cdfjplus.c \
 colour.c \
 custom.c \
 draw.c \
 main.c \
 savekey.c \
 sound.c \
 gameState_Copyright.c \
 gameState_CouchCompliant.c \
 gameState_DetectConsole.c \
 gameState_Menu.c \
 gameState_Rainbow.c \
 \
 grid6.c

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
HDRS = $(SRCS:.c=.h)


vpath %.c $(BASE) $(CUSTOM)
vpath %.S $(BASE) $(CUSTOM)

%.o: %.c %.h $(BASE)/$(DASM_TO_C)
	$(CC) $(CFLAGS) -I$(BASE) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -I$(BASE) -c $< -o $@


###############################################################################
# Default target (first in file) will build with 'make'

.PHONY: all
all: tools make_rom run_emulator
	mv *.jpg screenshots 2>/dev/null || true

###############################################################################
# EMULATOR = gopher|stella  -- or can be absent
# for gopher,  console type: -tv = NTSC|PAL|PAL60|SECAM

EMULATOR=gopher

.PHONY: run_emulator
run_emulator:

ifneq ($(EMULATOR),)
	pkill -f $(GOPHERNAME) || true

ifeq ($(EMULATOR), gopher)
	tmux new-session -d -s $(EMULATOR) $(GOPHER)/$(GOPHERNAME) -tv PAL60 -right savekey -dwarf $(CUSTOMELF) $(OUTPUT)/$(CUSTOMNAME).bin
endif

ifeq ($(EMULATOR), stella)
	tmux new-session -d -s $(EMULATOR) open -a $(STELLA) $(OUTPUT)/$(CUSTOMNAME).bin
endif

endif

###############################################################################



# -----------------------------------------------------------------------------
# IMPORTANT BUILD IDEA
# -----------------------------------------------------------------------------
# The final ROM needs main/bin/testarm.bin because the DASM source INCBINs it.
# The ARM/C code needs main/defines_dasm.h because it includes DASM-generated
# labels/constants.
#
# So a clean build must be done in stages:
#   1. prep              - build tools if needed, create folders, create dummy ARM bin
#   2. bootstrap_defines - run DASM once using the dummy ARM bin, generate defines_dasm.h
#   3. arm              - compile/link ARM C, produce the real testarm.bin
#   4. final            - run DASM again, now with the real testarm.bin
#
# .NOTPARALLEL prevents make -j from trying to do these stages out of order.
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# Stage 1: prep
# -----------------------------------------------------------------------------
# Build c_start if needed, create output folders, run c_start, and create a dummy
# ARM binary so the first DASM pass can satisfy the INCBIN line.

.PHONY: prep
prep: tools
	mkdir -p $(BIN) $(OUTPUT)
# 	$(TOOLS_BIN)/$(CSTART) ./$(SOURCE).asm $(CUSTOM)/custom.boot.lds
	@if [ ! -f "$(CUSTOMBIN)" ]; then \
		printf '\000' > "$(CUSTOMBIN)"; \
	fi

# -----------------------------------------------------------------------------
# Stage 2: bootstrap_defines
# -----------------------------------------------------------------------------
# Run DASM once using the dummy ARM binary. This is NOT the final build. This
# pass exists only to generate main/defines_dasm.h from DASM output/symbols.

.PHONY: bootstrap_defines
bootstrap_defines: gfx
	$(DASM) $(SOURCE).asm -f3 -v5 \
		-s$(OUTPUT)/$(SOURCE).sym \
		-l$(OUTPUT)/$(SOURCE).lst \
		-o$(OUTPUT)/$(SOURCE).bin
	@echo "// Auto-generated from DASM output and symbols" > $(BASE)/$(DASM_TO_C).tmp
	@echo "" >> $(BASE)/$(DASM_TO_C).tmp
	$(TOOLS_BIN)/$(CSTART) ./$(SOURCE).asm $(CUSTOM)/custom.boot.lds
	awk '$$1 ~ /^_/ {printf "#define %-25s 0x%s\n", $$1, $$2}' \
		$(OUTPUT)/$(SOURCE).sym >> $(BASE)/$(DASM_TO_C).tmp
	mv $(BASE)/$(DASM_TO_C).tmp $(BASE)/$(DASM_TO_C)

# -----------------------------------------------------------------------------
# Stage 3: arm
# -----------------------------------------------------------------------------
# Build the real ARM binary using the defines generated by the bootstrap pass.

.PHONY: arm
arm: $(CUSTOMOBJS) $(OBJS)
	rm -f $(CUSTOMBIN)
	$(MAKE) $(CUSTOMTARGETS)
	

# %.o: $(BASE)/%.c $(BASE)/$(DASM_TO_C)
# 	$(CC) $(CFLAGS) -c $< -o $@

$(CUSTOMELF): $(CUSTOMOBJS) Makefile $(CUSTOMLINK)
	$(CC) $(CFLAGS) -o $(CUSTOMELF) $(CUSTOMOBJS) $(OBJS) -T $(CUSTOMLINK) \
		-nostartfiles -Wl,-Map=$(CUSTOMMAP),--gc-sections
	$(TOOLCHAIN)-objdump -Sdrl $(CUSTOMELF) > $(OUTPUT)/$(CUSTOMNAME).obj

$(CUSTOMBIN): $(CUSTOMELF)
	$(OBJCOPY) -O binary -S $(CUSTOMELF) $(CUSTOMBIN)
	$(SIZE) $(CUSTOMOBJS) $(CUSTOMELF)

# -----------------------------------------------------------------------------
# Stage 4: final
# -----------------------------------------------------------------------------
# Rebuild the final 6502 ROM now that testarm.bin is real.

DATED_ROM := $(shell date +"$(CUSTOMNAME)_%Y%m%d@%H:%M:%S" | tr ' :' '__')

.PHONY: make_rom
make_rom: prep
	$(MAKE) bootstrap_defines
	$(MAKE) arm
	@ls -la $(CUSTOMBIN)
	@ls -la $(CUSTOMELF)
	@echo ">>> ARM binary size: $$(ls -la $(CUSTOMBIN))"
	$(DASM) $(SOURCE).asm -f3 -v5 \
		-s$(OUTPUT)/$(CUSTOMNAME).sym \
		-l$(OUTPUT)/$(CUSTOMNAME).lst \
		-o$(OUTPUT)/$(CUSTOMNAME).bin
	mkdir -p ROMs
	cp $(OUTPUT)/$(CUSTOMNAME).bin ROMs/$(DATED_ROM).bin


###############################################################################
# Tool builds

CSTART = c_start
WAV2RAW = wav2raw2600

TOOLS_SRC = $(BASE)/tools
TOOLS_BIN = $(TOOLS_SRC)/bin

.PHONY: tools
tools: $(TOOLS_BIN)/$(CSTART) $(TOOLS_BIN)/$(WAV2RAW)

$(TOOLS_BIN)/$(CSTART): $(TOOLS_SRC)/$(CSTART).c
	mkdir -p $(TOOLS_BIN)
	gcc -o $@ $<
	strip $@

$(TOOLS_BIN)/$(WAV2RAW): $(TOOLS_SRC)/$(WAV2RAW).c
	mkdir -p $(TOOLS_BIN)
	gcc -o $@ $<
	strip $@

###############################################################################
# Graphics

gfx: GRID6

GRID6: gfx/grid/*.gif
	(cd gfx/grid & python3 tools/1bpp.py -o grid6 gfx/grid/*.gif)
	mv grid6.* main


###############################################################################
# Cleaning

TRANSIENTS = $(CUSTOMTARGETS) \
			 $(BIN)/* \
			 $(OUTPUT)/* \
			 $(BASE)/$(DASM_TO_C) \
			 $(TOOLS_BIN)/*

.PHONY: clean
clean:
	rm -f $(TRANSIENTS)

###############################################################################

-include $(DEPS)
