/***************************************************/
/***************** c_start utility *****************/
/***************************************************/
/*      Craig Daniels - Gamax Software - 2026      */
/***************************************************/
/* Will automatically read the following values    */
/* from the <firstparam> ASM file                  */
/*     _DISPLAY_SIZE                                */
/*     ROM_SIZE                                    */
/*     C_START                                     */
/* and update ORIGIN and LENGTH values in the      */
/* users <secondparam> custom.boot.lds             */
/*     boot    ORIGIN                              */
/*     C_code  ORIGIN & LENGTH                     */
/*     ram     ORIGIN & LENGTH                     */
/***************************************************/

#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************** Functions ****************************/
void to_upper(char *s);

/**************************** Variables ****************************/
bool found_display_size = false;
bool found_rom_size = false;
bool found_c_start = false;

unsigned int display_size = 0;
unsigned int rom_size = 0;
unsigned char c_start = '0';    // default

int c_start_val = 0;
int c_size = 0;
int ram_size = 0;
int ram_free = 0;
int ram_start = 0;

const unsigned int BOOT_SIZE = 0x60;

// The ARM C stack (C_STACK, set in the game's own .asm -- e.g. cdfj+_template.asm) starts at a
// FIXED address near the top of the same physical RAM this tool carves "ram" (.data/.bss) out of,
// and grows DOWNWARD from there into that same space. Before this reserve existed, `ram`'s LENGTH
// was computed with zero regard for that -- .data/.bss was allowed to grow to within ~36 bytes of
// C_STACK's own STARTING address, i.e. to within ~36 bytes of before the stack has pushed a single
// byte. Any real call depth at all (this is a live risk, not theoretical: it caused visible cave-
// data corruption once real .data/.bss usage got close enough -- see git history around
// 2026-08-07) then overwrites .data/.bss.
//
// STACK_RESERVE carves out a fixed block below C_STACK that `ram_free` (and so "ram"'s LENGTH,
// below) is NOT allowed to use, converting "silent runtime corruption once real stack depth
// exceeds whatever's left" into "a hard, obvious link error the moment .data/.bss growth eats
// into the reserve" -- the same way the existing `ram_free < 36` check already guards the display
// buffer/RAM split, just extended to guard the stack too.
//
// 256 bytes of ACTUAL margin below C_STACK's own starting address is the target, chosen from an
// actual -fstack-usage sweep of every function in the codebase at the time (2026-08-07) -- no
// recursion anywhere, largest single function frame 160 bytes, the vast majority under 56 bytes --
// so even a generously-deep (10+ level) real call chain comfortably fits. NOT a proven worst-case
// bound (that would need a real call-graph depth analysis, including through the game state/kernel
// function-pointer dispatch tables, which this quick pass didn't attempt) -- if a future stack-
// related corruption bug shows up again despite this reserve, that's the next thing to build. If
// this constant needs to grow later, that will show up as a normal "ERROR: Not enough RAM" from
// ram_free's own existing check below, not as a mystery runtime bug -- exactly the tradeoff this
// whole reserve exists for.
//
// The constant itself is 292, not 256: C_STACK sits a fixed 36 bytes below the absolute top of
// whatever total RAM this build's ram_size resolves to (confirmed the same 36-byte gap across
// every ROM_SIZE's C_STACK value in cdfj+_template.asm -- an existing, presumably IAR-toolchain-
// related reserve, unrelated to this one), and ram_free is computed relative to that same
// absolute top, not relative to C_STACK directly -- so 36 has to be added back on top of the
// real 256-byte target to actually land the enforced boundary 256 bytes below C_STACK itself.
#define STACK_RESERVE (256 + 36)

/***************************************************/
bool verbose = false;    // to control extra data output
/***************************************************/

/************************************************* Begin
 * *************************************************/
int main(int argc, char *argv[]) {

    /**************************** Check Parameters ****************************/
    if (argc < 3) {
        printf("Usage: c_start <asm_file> <lds_file>\n");
        return 1;
    }

    const char *asm_file = argv[1];
    const char *lds_file = argv[2];

    FILE *f;

    /**************************** Check for Files ****************************/
    // Check ASM file
    f = fopen(asm_file, "r");
    if (!f) {
        printf("ERROR: ASM file NOT found: %s\n", asm_file);
        return 1;
    }
    if (verbose)
        printf("ASM file found: %s\n", asm_file);
    fclose(f);

    // Check LDS file
    f = fopen(lds_file, "r");
    if (!f) {
        printf("ERROR: LDS file NOT found: %s\n", lds_file);
        return 1;
    }
    if (verbose)
        printf("LDS file found: %s\n", lds_file);
    fclose(f);

    char line[512];
    char test[512];

    // Scan symbol table for C_CODE
    f = fopen("output/cdfj+_template.sym", "r");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "C_CODE ")) {
                c_start = line[25];
                found_c_start = true;
                break;
            }
        }
    }

    // Scan ASM file

    /**************************** Open/Read ASM File
     * ****************************/
    f = fopen(asm_file, "r");
    if (!f) {
        printf("ERROR: Could not open ASM file.\n");
        return 1;
    }

    while (fgets(line, sizeof(line), f)) {
        strcpy(test, line);
        to_upper(test);

        char *p = test;

        // Skip leading whitespace (spaces, tabs, etc.)
        while (isspace((unsigned char)*p))
            p++;

        /* calculate C_START via $800 + ASM_BANKS * $1000 */
        // requires format "ASM_BANKS =" (exactly)

        // char *banks = strstr(p, "ASM_BANKS =");
        // if (banks) {
        // 	int bank_count = strtol(banks + 12, NULL, 10);
        // 	int start = 0x800 + bank_count * 0x1000;
        // 	char buf[32];
        // 	sprintf(buf, "%x", start);
        // 	c_start = toupper(buf[0]);
        // 	c_start_val = (c_start >= 'A') ? c_start - 'A' + 10 : c_start - '0';
        // 	printf("%d\n", c_start_val);
        // 	found_c_start = true;
        // }

        // /**************************** Read C_START
        // ****************************/ if ((strncmp(p, "C_START", 7) == 0) &&
        // (!found_c_start)) { 	if (verbose) 		printf("FOUND C_START LINE:
        // %s", line); 	found_c_start = true;

        // 	char *eq = strchr(p, '=');

        // 	if (eq) {
        // 		eq++; // past '='

        // 		while (isspace((unsigned char)*eq))
        // 			eq++;

        // 		if (*eq == '$') {
        // 			eq++; // skip '$'
        // 		}

        // 		c_start = *eq;							   // first hex digit
        // 		c_start = toupper((unsigned char)c_start); // normalize

        // 		if (c_start < '1' || c_start > '7') {
        // 			printf("ERROR: C_START MS nybble out of range (must be "
        // 				   "1-7): %c\n",
        // 				   c_start);
        // 			return 1;
        // 		}

        // 		c_start_val = c_start - '0';

        // 		if (verbose)
        // 			printf("C_START MS nybble = %c\n", c_start);
        // 	} else {
        // 		printf("ERROR: C_START line missing '='\n");
        // 		return 1;
        // 	}
        // }

        /**************************** Read ROM Size
         * ****************************/
        if ((strncmp(p, "ROM_SIZE", 8) == 0) && (!found_rom_size)) {
            if (verbose)
                printf("FOUND ROM_SIZE LINE: %s", line);
            found_rom_size = true;

            char *eq = strchr(p, '=');

            if (eq) {
                eq++;    // move past '='

                while (isspace((unsigned char)*eq))
                    eq++;

                rom_size = strtol(eq, NULL, 0);

                if (!(rom_size == 32 || rom_size == 64 || rom_size == 128 || rom_size == 256 || rom_size == 512)) {
                    printf("ERROR: ROM_SIZE must be 32, 64, 128, 256, or 512. "
                           "Found: %d\n",
                           rom_size);
                    return 1;
                }

                if (verbose)
                    printf("ROM_SIZE value = %d\n", rom_size);
            } else {
                printf("ERROR: ROM_SIZE line missing '='\n");
                return 1;
            }
        }

        /**************************** Read DISPLAY_SIZE
         * ****************************/
        if ((strncmp(p, "_DISPLAY_SIZE", 12) == 0) && (!found_display_size)) {
            if (verbose)
                printf("FOUND _DISPLAY_SIZE LINE: %s", line);
            found_display_size = true;

            char *eq = strchr(p, '=');

            if (eq) {
                eq++;    // move past '='

                // skip whitespace after '='
                while (isspace((unsigned char)*eq))
                    eq++;

                display_size = strtol(eq, NULL, 0);

                if (verbose)
                    printf("DISPLAY_SIZE value = %d\n", display_size);
            } else {
                printf("ERROR: DISPLAY_SIZE line missing '='\n");
                return 1;
            }
        }
    }

    fclose(f);

    /**************************** Ensure All Found ****************************/
    if (!(found_c_start && found_display_size && found_rom_size)) {
        printf("ERROR: Unable to locate...\n");
        if (!found_c_start) {
            printf("  C_START\n");
        }
        if (!found_rom_size) {
            printf("  ROM_SIZE\n");
        }
        if (!found_display_size) {
            printf("  DISPLAY_SIZE\n");
        }
        printf("inside ASM file %s\n", asm_file);
        return 1;
    }

    c_size = (rom_size - 2 - (4 * c_start_val)) * 1024 - BOOT_SIZE;
    if (verbose)
        printf("c_size %d\n", c_size);

    switch (rom_size) {
    case 32:
        ram_size = 8;
        break;
    case 64:
        ram_size = 16;
        break;
    case 128:
        ram_size = 16;
        break;
    case 256:
        ram_size = 32;
        break;
    case 512:
        ram_size = 32;
        break;
    }
    if (verbose)
        printf("ram_size %d\n", ram_size);

    ram_free = ((ram_size - 2) * 1024) - display_size - STACK_RESERVE;
    if (verbose)
        printf("ram_free %d\n", ram_free);

    if (ram_free < 36) {
        printf("ERROR: Not enough RAM for current configuration\n");
        printf("RAM free after display allocation and system reserve: %d bytes\n", ram_free - 35);
        return 1;
    }

    if (ram_free < 512) {
        printf("WARNING: RAM low - %d bytes free after display allocation and "
               "system reserve\n",
               (ram_free - 35));
    }

    ram_start = 2048 + display_size;
    if (verbose)
        printf("ram_start %d\n", ram_start);

    char temp_file[512];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", lds_file);

    FILE *in = fopen(lds_file, "r");
    FILE *out = fopen(temp_file, "w");

    if (!in) {
        printf("ERROR: Could not open LDS file for reading: %s\n", lds_file);
        return 1;
    }

    if (!out) {
        fclose(in);
        printf("ERROR: Could not create temp file: %s\n", temp_file);
        return 1;
    }

    while (fgets(line, sizeof(line), in)) {
        strcpy(test, line);
        to_upper(test);

        char *p = test;
        while (isspace((unsigned char)*p))
            p++;

        if (strncmp(p, "BOOT", 4) == 0) {
            fprintf(out,
                    "    boot (RX)   : ORIGIN = 0x%c800    , LENGTH = "
                    "0x60\t\t/* C-runtime booter */\n",
                    c_start);
        } else if (strncmp(p, "C_CODE", 6) == 0) {
            fprintf(out,
                    "    C_code (RX) : ORIGIN = 0x%c860    , LENGTH = "
                    "0x%05X\t\t/* C code size */\n",
                    c_start, c_size);
        } else if (strncmp(p, "RAM", 3) == 0) {
            fprintf(out,
                    "    ram         : ORIGIN = 0x4000%04X, LENGTH = "
                    "0x%04X\t\t/* free RAM */    \n",
                    ram_start, ram_free);
        } else {
            fputs(line, out);
        }
    }

    fclose(in);
    fclose(out);

    if (remove(lds_file) != 0) {
        printf("ERROR: Could not delete original LDS file: %s\n", lds_file);
        return 1;
    }

    if (rename(temp_file, lds_file) != 0) {
        printf("ERROR: Could not rename temp file to LDS file: %s\n", lds_file);
        return 1;
    }

    printf("LDS file updated successfully.\n");

    return 0;
}

/*******************************************************************/
/**************************** Functions ****************************/
void to_upper(char *s) {
    while (*s) {
        *s = toupper((unsigned char)*s);
        s++;
    }
}