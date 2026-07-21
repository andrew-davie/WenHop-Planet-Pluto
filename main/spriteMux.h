#pragma once

// GRP0/GRP1 time-multiplexed sprite scheduler.
// See SPRITE_MULTIPLEX_DESIGN.md for the full writeup -- this is the ARM-side
// half only (the scheduler + per-scanline stream buffers). The 6502 kernel
// loop that consumes muxGRP0/muxHMP0/etc is NOT part of this file; it
// belongs in the 6507 kernel bank, which wasn't available in the session
// this was written in.
//
// REPOSITIONING MODEL (revised): no RESP0/RESP1 coarse jumps during the
// visible kernel, full stop -- a RESP delay loop monopolizes the CPU for
// most of a scanline, which is incompatible with also servicing playfield
// streams on that same line. Every lane's *first* object of the frame gets
// a free placement via the existing vblank _P0_X/_P1_X mechanism
// (drawPlayerSprite() already does exactly this, once/frame, before the
// visible kernel starts). Every object after that, on that lane, is reached
// purely by HMOVE crawl at <=7px/scanline -- no scanline is ever skipped or
// blanked for repositioning; a lane just shows no shape (GRP=0) on the
// scanlines it's mid-crawl, while the rest of that scanline's normal
// per-line work (playfield, COLUP0/1, the other lane) proceeds unchanged.

#include <stdbool.h>

#define MUX_MAX_OBJECTS 32    // max objects trackable/submittable in one frame
#define MUX_MAX_STEP 7        // max |dx| coverable by a single HMOVE nudge
#define MUX_NO_SHAPE 0        // muxGRP0/muxGRP1 value meaning "draw nothing"

// Priority 0 objects are never dropped by the scheduler (reserve for things
// like the player itself, if you ever route it through this system). Higher
// values are increasingly droppable relative to their starvation count --
// see muxSchedule()'s tiebreak. NOTE: not yet enforced by the scheduler --
// see the comment at its one drop site.
#define MUX_PRIORITY_NEVER_DROP 0

typedef struct {
    unsigned char id;      // 0..MUX_MAX_OBJECTS-1, STABLE across frames -- this is
                            // what starvation tracking is keyed on, not array position
    unsigned char x;       // desired coarse screen X, 0..159
    unsigned short yTop;   // first scanline this object occupies, 0.._SCANLINES-1
    unsigned char height;  // scanline count (yTop+height must be <= _SCANLINES)
    const unsigned char *shape;   // `height` bytes, one GRP pattern per scanline
    const unsigned char *colour;  // `height` bytes, one COLUP value per scanline, or NULL
    unsigned char fixedColour;    // used for every line when colour == NULL
    unsigned char priority;       // MUX_PRIORITY_NEVER_DROP, or higher = more droppable
} MuxObject;

// Every other kernel in this codebase writes its per-scanline data into the
// shared DDR RAM window at pre-allocated, _BUFFER_SIZE-spaced offsets
// (_BUF_GAME_GRP0 etc in defines_dasm.h) -- that's the only memory the 6507
// kernel side can actually read from. This module doesn't invent new offsets
// of its own (that needs a linker/kernel-side allocation this session didn't
// have access to) -- the caller supplies them, same pattern as
// drawScreenMirror(int buffer) in drawScreen.c.
//
// GRP0/GRP1/COLUP0/COLUP1 can just be _BUF_GAME_GRP0/_BUF_GAME_GRP1/
// _BUF_GAME_COLUP0/_BUF_GAME_COLUP1 outright (same streams drawPlayerSprite()
// already uses) if this replaces that call for a given frame. HMP0/HMP1 are
// the only genuinely NEW streams -- 2 offsets, 2 new fixed-timing per-scanline
// register pokes on the kernel side, no branching required (every scanline
// runs the identical instruction sequence, just sometimes with HMP0/HMP1
// nonzero). That's the whole kernel-side ask now that RESP is out of the
// picture.
typedef struct {
    int grp0, grp1;      // shape byte per scanline
    int colup0, colup1;  // colour byte per scanline
    int hmp0, hmp1;      // pre-shifted HMOVE nibble per scanline, 0 = no motion
} MuxBuffers;

// Call once per frame before submitting objects.
void muxBeginFrame();

// Submit one candidate object for this frame. Cheap, just records it -- the
// real scheduling work happens in muxSchedule(). Call this for every
// currently-relevant object (already Y/X culled to onscreen-ish by the
// caller); returns false if the MUX_MAX_OBJECTS table is already full.
bool muxAdd(const MuxObject *obj);

// Runs the greedy 2-lane scheduler over everything submitted this frame via
// muxAdd(), fills the 6 RAM streams described by `buffers` for the whole
// _SCANLINES range, and writes each used lane's first-object X directly to
// RAM[_P0_X]/RAM[_P1_X] (the existing vblank-applied coarse position, same
// mechanism drawPlayerSprite() uses) -- so this call is only valid to make
// during vblank, before RAM[_P0_X]/RAM[_P1_X] get latched by RESP0/RESP1,
// and it assumes it owns P0/P1 for this frame (don't also call
// drawPlayerSprite() the same frame, they'll fight over the same registers).
void muxSchedule(const MuxBuffers *buffers);

// EOF
