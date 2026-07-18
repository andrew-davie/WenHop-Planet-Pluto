#pragma once

extern void *DDR;

#define RAM ((unsigned char *)DDR)
#define RAM_2B ((unsigned short int *)DDR)
#define RAM_4B ((unsigned int *)DDR)

#define ROM ((unsigned char *)0)
#define ROM_2B ((unsigned short int *)0)
#define ROM_4B ((unsigned int *)0)

// CDFJ+ DataStream Pointers
#define DS0PTR 0
#define DS1PTR 1
#define DS2PTR 2
#define DS3PTR 3
#define DS4PTR 4
#define DS5PTR 5
#define DS6PTR 6
#define DS7PTR 7
#define DS8PTR 8
#define DS9PTR 9
#define DS10PTR 10
#define DS11PTR 11
#define DS12PTR 12
#define DS13PTR 13
#define DS14PTR 14
#define DS15PTR 15
#define DS16PTR 16
#define DS17PTR 17
#define DS18PTR 18
#define DS19PTR 19
#define DS20PTR 20
#define DS21PTR 21
#define DS22PTR 22
#define DS23PTR 23
#define DS24PTR 24
#define DS25PTR 25
#define DS26PTR 26
#define DS27PTR 27
#define DS28PTR 28
#define DS29PTR 29
#define DS30PTR 30
#define DS31PTR 31
#define DSCOMM_PTR 32
#define DSJMP1PTR 33
#define DSJMP2PTR 34
#define AMPLITUDE_PTR 35

// Queue variables
extern unsigned int *const _QPTR;
extern unsigned int *const _QINC;
extern unsigned int *const _WAVEFORM;

// Set fetcher pointer (offset from start of display data)
void setPointer(const int fetcher, const unsigned int offset);

// clang-format off

/* Timer 1 */
#define T1IR            (*((volatile unsigned long *) 0xE0008000))
#define T1TCR           (*((volatile unsigned long *) 0xE0008004))
#define T1TC            (*((volatile unsigned long *) 0xE0008008))
#define T1PR            (*((volatile unsigned long *) 0xE000800C))
#define T1PC            (*((volatile unsigned long *) 0xE0008010))
#define T1MCR           (*((volatile unsigned long *) 0xE0008014))
#define T1MR0           (*((volatile unsigned long *) 0xE0008018))
#define T1MR1           (*((volatile unsigned long *) 0xE000801C))
#define T1MR2           (*((volatile unsigned long *) 0xE0008020))
#define T1MR3           (*((volatile unsigned long *) 0xE0008024))
#define T1CCR           (*((volatile unsigned long *) 0xE0008028))
#define T1CR0           (*((volatile unsigned long *) 0xE000802C))
#define T1CR1           (*((volatile unsigned long *) 0xE0008030))
#define T1CR2           (*((volatile unsigned long *) 0xE0008034))
#define T1CR3           (*((volatile unsigned long *) 0xE0008038))
#define T1EMR           (*((volatile unsigned long *) 0xE000803C))
#define T1CTCR          (*((volatile unsigned long *) 0xE0008070))

#define APBDIV          (*((volatile unsigned long *) 0xE01FC100))

// Set fetcher increment
void setIncrement(const int fetcher, const unsigned char whole,
                         const unsigned char frac);

// Set DA sample address
void setSamplePtr(unsigned int address);

// Set note frequency
void setNote(int note, unsigned int freq);

// Reset waveform
void resetWave(int wave);

// Get waveform pointer
unsigned int getWavePtr(int wave);

// Set waveform size:
// 20 = 4096 bytes
// 21 = 2048 bytes (DEFAULT)
// 22 = 1024 bytes
// 23 = 512 bytes
// 24 = 256 bytes
// 25 = 128 bytes
// 26 = 64 bytes
// 27 = 32 bytes
// 28 = 16 bytes
// 29 = 8 bytes
// 30 = 4 bytes
// 31 = 2 bytes

void setWaveSize(int wave, unsigned int size);


// Pitch table
extern const unsigned int _pitchTable[12];

// Calculate frequency for note
unsigned int getPitch(unsigned int note);


// Set memory area to fill value
void myMemset(unsigned char *destination, unsigned int fill,
                     unsigned int count);

// Copy memory from source to destination
void myMemcpy(unsigned char *destination, unsigned char *source,
                     unsigned int count);

// Set memory area to fill value
// in theory 4x faster than myMemset(), but data must be WORD (4 byte) aligned
void myMemsetInt(unsigned int *destination, unsigned int fill,
                        unsigned int count);

// Copy memory from source to destination
// in theory 4x faster than myMemset(), but data must be WORD (4 byte) aligned
void myMemcpyInt(unsigned int *destination, unsigned int *source,
                        unsigned int count);
// EOF
