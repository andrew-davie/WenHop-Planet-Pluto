#include "spriteMux.h"

#include "defines_dasm.h"

#include "cdfjplus.h"

// See SPRITE_MULTIPLEX_DESIGN.md for the full rationale. Summary of the
// scheduling model: 2 hardware lanes (GRP0, GRP1), each object is a fixed
// [yTop, yTop+height) window (Y can't be time-shifted, it's tied to the
// raster). A lane's first object of the frame is free (placed via the
// existing vblank RESP0/RESP1, same as drawPlayerSprite() already does).
// Every object after that costs ceil(|dx|/MUX_MAX_STEP) scanlines of pure
// HMOVE crawl -- no RESP during the visible kernel, so no scanline is ever
// skipped for playfield purposes; a crawling lane just shows no shape on
// those lines while everything else (playfield, the other lane) proceeds
// as normal.

static const MuxObject *objTable[MUX_MAX_OBJECTS];
static int objCount;

// Keyed by MuxObject.id (stable across frames), not by array position --
// this is what makes the starvation tiebreak in muxSchedule() work: an
// object that keeps losing scheduling contention accumulates a rising
// count here until it starts outranking fresher competitors.
static unsigned char framesSinceShown[MUX_MAX_OBJECTS];

typedef struct {
    int x;         // current coarse X of this lane; -1 = not yet used this frame
    int readyAt;   // next scanline this lane is free
} Lane;


void muxBeginFrame() {
    objCount = 0;
}

bool muxAdd(const MuxObject *obj) {

    if (objCount >= MUX_MAX_OBJECTS)
        return false;

    objTable[objCount++] = obj;
    return true;
}

// Insertion sort by yTop ascending; ties broken by starvation (longest-
// unshown first). objCount is <= MUX_MAX_OBJECTS (32) so O(n^2) is fine --
// this runs once per frame on the ARM side, not per scanline.
static void sortObjects() {

    for (int i = 1; i < objCount; i++) {

        const MuxObject *key = objTable[i];
        unsigned char keyStarve = framesSinceShown[key->id];

        int j = i - 1;
        while (j >= 0 &&
               (objTable[j]->yTop > key->yTop ||
                (objTable[j]->yTop == key->yTop && framesSinceShown[objTable[j]->id] < keyStarve))) {

            objTable[j + 1] = objTable[j];
            j--;
        }
        objTable[j + 1] = key;
    }
}

static int absInt(int v) {
    return v < 0 ? -v : v;
}

// Scanlines of pure-HMOVE crawl needed to cover `dx` (signed, target - current),
// at up to MUX_MAX_STEP per line. 0 if the lane hasn't been used yet this
// frame (free vblank placement instead -- see muxSchedule()).
static int crawlLines(int laneX, int targetX) {

    if (laneX < 0)
        return 0;

    int dx = absInt(targetX - laneX);
    if (dx == 0)
        return 0;

    return (dx + MUX_MAX_STEP - 1) / MUX_MAX_STEP;    // ceil(dx / MUX_MAX_STEP)
}

// HMPx registers hold a signed 4-bit value in their top nibble (two's
// complement); positive = move left, negative = move right (verified
// against the Stella Programmer's Guide -- see design doc). `step` must be
// in [-7, 7] here.
static unsigned char hmValue(int step) {
    return (unsigned char)((step & 0x0F) << 4);
}

// Writes `n` scanlines of HMOVE crawl into hmpOff, ending at (startLine+n-1),
// covering the full signed distance `dx` a MUX_MAX_STEP bite at a time
// (front-loaded: full 7px steps until the remainder is < 7).
static void writeCrawl(int dx, int n, int hmpOff, int startLine) {

    int remaining = dx;
    for (int i = 0; i < n; i++) {

        int step = remaining;
        if (step > MUX_MAX_STEP)
            step = MUX_MAX_STEP;
        else if (step < -MUX_MAX_STEP)
            step = -MUX_MAX_STEP;

        RAM[hmpOff + startLine + i] = hmValue(step);
        remaining -= step;
    }
}

// Writes one scheduled object's shape/colour for [yTop, yTop+height), plus
// (if crawling in) the HMOVE crawl on the `transitLines` scanlines
// immediately before yTop.
static void emitObject(const MuxObject *o, int laneX, int transitLines, int grpOff, int colupOff, int hmpOff) {

    if (transitLines > 0) {
        int startLine = (int)o->yTop - transitLines;
        writeCrawl((int)o->x - laneX, transitLines, hmpOff, startLine);
    }

    unsigned char *grp = RAM + grpOff + o->yTop;
    unsigned char *colup = RAM + colupOff + o->yTop;

    for (int line = 0; line < o->height; line++) {
        grp[line] = o->shape[line];
        colup[line] = o->colour ? o->colour[line] : o->fixedColour;
    }
}

void muxSchedule(const MuxBuffers *buffers) {

    for (int i = 0; i < _SCANLINES; i++) {

        RAM[buffers->grp0 + i] = MUX_NO_SHAPE;
        RAM[buffers->grp1 + i] = MUX_NO_SHAPE;
        RAM[buffers->colup0 + i] = 0;
        RAM[buffers->colup1 + i] = 0;
        RAM[buffers->hmp0 + i] = 0;
        RAM[buffers->hmp1 + i] = 0;
    }

    sortObjects();

    Lane lane0 = {-1, 0};
    Lane lane1 = {-1, 0};

    for (int i = 0; i < objCount; i++) {

        const MuxObject *o = objTable[i];

        if ((int)o->yTop + o->height > _SCANLINES || o->height == 0)
            continue;    // malformed submission -- skip rather than corrupt neighbouring lines

        int n0 = crawlLines(lane0.x, o->x);
        int n1 = crawlLines(lane1.x, o->x);

        bool fits0 = ((int)o->yTop - lane0.readyAt) >= n0;
        bool fits1 = ((int)o->yTop - lane1.readyAt) >= n1;

        int chosen;
        if (fits0 && fits1)
            chosen = (n0 <= n1) ? 0 : 1;    // prefer whichever needs less crawl
        else if (fits0)
            chosen = 0;
        else if (fits1)
            chosen = 1;
        else
            chosen = -1;

        if (chosen < 0) {
            // Neither lane can make it in time -- this is the flicker case.
            // priority 0 objects are "never drop" by contract; a caller that
            // relies on that should keep their count/placement modest enough
            // that this branch never has to violate it (it doesn't, on
            // purpose -- MUX_PRIORITY_NEVER_DROP is a submission-side
            // discipline, not something this scheduler special-cases).
            if (framesSinceShown[o->id] < 255)
                framesSinceShown[o->id]++;
            continue;
        }

        if (chosen == 0) {
            emitObject(o, lane0.x, n0, buffers->grp0, buffers->colup0, buffers->hmp0);
            lane0.x = o->x;
            lane0.readyAt = o->yTop + o->height;
        } else {
            emitObject(o, lane1.x, n1, buffers->grp1, buffers->colup1, buffers->hmp1);
            lane1.x = o->x;
            lane1.readyAt = o->yTop + o->height;
        }

        framesSinceShown[o->id] = 0;
    }

    // Each lane's first object this frame (if any) is placed for free via
    // the existing vblank coarse-position mechanism -- same registers
    // drawPlayerSprite() already writes, applied by RESP0/RESP1 before the
    // visible kernel starts, not during it.
    if (lane0.x >= 0)
        RAM[_P0_X] = (unsigned char)lane0.x;
    if (lane1.x >= 0)
        RAM[_P1_X] = (unsigned char)lane1.x;
}

// EOF
