#include "animations.h"
#include "defines_dasm.h"

#include "gameState.h"

#include "cdfjplus.h"

#include "animations.h"
#include "board.h"
#include "caveData.h"
#include "colour.h"
#include "decodeCaves.h"
#include "draw.h"
#include "drawPlayer.h"
#include "drawScreen.h"
#include "gameState.h"
#include "joystick.h"
#include "kernels.h"
#include "main.h"
#include "mellon.h"
#include "particle.h"
#include "playerAnimation.h"
#include "random.h"
#include "schedule.h"
#include "score.h"
#include "scroll.h"
#include "sound.h"
#include "swipe.h"
#include "wyrm.h"


void initDataStreams_Game() {

    static const struct dataStreams streams[] = {

        {_DS_GAME_PF0_LEFT, _BUF_GAME_PF0_LEFT},
        {_DS_GAME_PF1_LEFT, _BUF_GAME_PF1_LEFT},
        {_DS_GAME_PF2_LEFT, _BUF_GAME_PF2_LEFT},
        {_DS_GAME_PF0_RIGHT, _BUF_GAME_PF0_RIGHT},
        {_DS_GAME_PF1_RIGHT, _BUF_GAME_PF1_RIGHT},
        {_DS_GAME_PF2_RIGHT, _BUF_GAME_PF2_RIGHT},

        {_DS_GAME_COLUPF, _BUF_GAME_COLUPF},
        {_DS_GAME_COLUBK, _BUF_GAME_COLUBK},
        {_DS_GAME_COLUP0, _BUF_GAME_COLUP0},
        {_DS_GAME_COLUP1, _BUF_GAME_COLUP1},
        {_DS_GAME_GRP0A, _BUF_GAME_GRP0},
        {_DS_GAME_GRP1A, _BUF_GAME_GRP1},

        {DSJMP1PTR, _BUF_GAME_JUMP},
    };

    initDataStreams(streams, sizeof(streams) / sizeof(struct dataStreams));
}


void initKernel_Game() {

    setJumpVectors(_BUF_GAME_JUMP, _gameLoop, _gameExit, _SCANLINES);
    initDataStreams_Game();
}


void initGameState_Game() {

    caveSequenceStarted = true;

    initNewGame();

    loadCave(cave);
}


// Shared by a genuine new-game entry (initGameState_Game() above, which calls initNewGame()
// first) and startTeleportWarp() (caveData.c), which calls this directly, mid-game, to switch
// to a different cave without resetting score/lives. Everything initGameState_Game() used to
// do apart from initNewGame() lives here unchanged.

void loadCave(int newCave) {

    cave = newCave;

    initBoard();
    initCharVector();
    initCharAnimations();

    initSprites();
    initParticles();
    initTool();

    initWyrms();     // todo: --> initNextLife
    initPlayer();    // --> initNextLife

#if ENABLE_SWIPE
    setSwipeType(SWIPE_STAR);
    initStarSwipe();    // clears to fully hidden and holds idle -- doesn't start growing yet,
                        // since playerX/Y aren't placed until the cave decode runs. decodeCaves.c
                        // calls setSwipe() for real, centred on the player, once that's known.
#endif

    exitMode = 0;    // --> initNextlife

    liquidTrixel_8 = (BOARD_TRIX_Y + CHAR_TRIX_Y) << 8;    // just under the board's bottom bound --
                                                           // matches caveData.c's own "off the bottom
                                                           // of the board" caves -- so decodeCave()'s
                                                           // theCave->water, or a scanned water/lava
                                                           // tile, always has something lower to pull
                                                           // this down to
    showLava = false;
    showWater = false;

    decodeCave(cave);    // TODO: in initNextLife instead


    luminance = -15;
    lumTarget = 0;
    loadPalette();    // redundant?  caves now hold palette

    setSchedule(SCHEDULE_UNPACK_CAVE);

    gameSpeed = SPEED_BASE;
    gameFrame = gameSpeed;    // force rollover

#if ENABLE_SHAKE
    setShake(0);
    shakeX = 0;
    shakeY = 0;
#endif


    gravity = 1;
    nextGravity = gravity;

    frame = 0;


    sound_volume = VOLUME_PLAYING;
}


void VB_Game() {

    // T1TC reset and the scheduledTasks() dispatch (was the last statement in this function) both
    // moved to the generic runARM_VerticalBlank() in main.c -- same effective timing (T1TC reset
    // right before this function is entered, scheduledTasks() called right after it returns), but
    // now shared by every state instead of being wired specifically into Game.

    updatePlayerAnimation();
    scroll();

#if ENABLE_SWIPE
    // Don't advance the swipe while the cave is still being decoded --
    // setSwipe() itself isn't called until scheduleUnpackCave() (schedule.c)
    // confirms decode is fully done, so swipe() would just be idling/holding
    // black anyway during SCHEDULE_UNPACK_CAVE. But it's not free to call:
    // it still burns some of this frame's time budget (the finish-clear
    // state machine, etc.), which is time taken away from
    // scheduleUnpackCave()'s own per-frame decode slice (see schedule.c's
    // "while (T1TC < availableIdleTime - 20000)"). Skipping it here lets
    // cave decode use the full frame budget instead of sharing it with an
    // idle swipe. applySwipeMask() below still runs unconditionally every
    // frame -- that's what forces the screen black while drawScreen() itself
    // is also skipped during unpack (see OS_Game()), regardless of whatever
    // stale buffer contents are sitting there.

    if (gameSchedule != SCHEDULE_UNPACK_CAVE)
        swipe(50000);    // Bumped from 35000, confirmed on hardware -- now safe to hold back more of the
                         // frame for other VB_Game systems without any visible cost, because circle()'s
                         // border is a real double buffer now (see swipe.c's borderShowA/B): a lap that
                         // takes more frames to get through (the direct result of giving swipe() a
                         // smaller slice here) just holds last lap's finished ring steady for longer
                         // instead of showing a half-drawn one. Before that fix, raising this value
                         // would have made the old bottom-of-circle flashing worse, not better.

#endif

    initDataStreams_Game();

    gameFrame++;

    if (attachmentFlashTicks)
        attachmentFlashTicks--;


#if ENABLE_SHAKE

    if (shakeTime) {

        shakeTime--;

        int lastShakeX = shakeX;
        int lastShakeY = shakeY;

        shakeX = (rangeRandom(3) - 1) << 16;
        shakeY = (rangeRandom(5) - 2) << 16;

        if (shakeX == lastShakeX && shakeY == lastShakeY) {
            shakeX += (1 << 16);
            if (shakeX >= 2 << 16)
                shakeX = -(1 << 16);
        }
    } else
        shakeX = shakeY = 0;

#endif

    if (RAM[_SWCHB] != 0x3F)
        setGameState(GS_MENU);

    processCharAnimations();
    driveTeleportSpin(teleportLocked);
    updateTeleportArrivalSwirl();

    setPalette(_BUF_GAME_COLUBK);

    if (gameSchedule != SCHEDULE_UNPACK_CAVE) {

        // // Always draw for exitMode -- playerX/playerY (what this and scroll() both track)
        // // update the instant the exit door is stepped on (mellon.c's checkHighPriorityMove),
        // // but moveHusk() -- which would normally move the board's own CH_MELLON_HUSK cell to
        // // match -- is deliberately skipped there so the door tile can show CH_EXITBLANK
        // // instead. That leaves a stale CH_MELLON_HUSK sitting in the board array at the
        // // player's *previous* cell for the rest of exitMode's countdown (board.c's
        // // TYPE_MELLON_HUSK case needs a cell of that type to keep finding on each scan, or
        // // the countdown -> setGameState(GS_MENU) never fires), and it's still drawn as a
        // // tile via AnimMellonHusk. Suppressing this sprite once exitMode settled left that
        // // stale, wrongly-positioned tile glyph as the only visible "player" on screen --
        // // frozen one cell behind, not following the camera in to the door. Keeping the
        // // correctly-tracked sprite visible throughout covers for it.
        // //
        // // teleportLocked doesn't have that problem -- moveHusk() is skipped for the teleport
        // // tile itself too (mellon.c), but the tile the player is standing on is exactly where
        // // they actually are the whole time (it just keeps showing its RAM-static glyph), so
        // // there's no stale/wrongly-positioned stand-in to cover for. drawPlayerSprite() itself
        // // hides the sprite once the walk-in glide settles (see its own teleportLocked check) --
        // // always called from here, unconditionally, so its GRP0 clear still runs every frame;
        // // skipping the call entirely from here left the last real frame's sprite bitmap sitting
        // // in GRP0 forever, frozen on screen instead of disappearing.
        // drawPlayerSprite();

        // Unconditional (not gated on !maskNeeded below) so floating text -- notably the
        // level-start ID string (schedule.c) -- draws into _BUF_GAME_PF0_LEFT throughout the
        // swipe reveal too, same as drawPlayerSprite() above: applySwipeMask() (end of this
        // function) then clips it to whatever the growing swipe has actually revealed so far,
        // letting the swipe progressively reveal the string instead of it popping in only once
        // the swipe fully completes. levelLabelTicks ticks down in lockstep right here, once
        // per call, so it always reaches zero on the exact frame the ID string's own particle
        // age does (see its comment in main.h) regardless of maskNeeded.

        if (levelLabelTicks)
            levelLabelTicks--;

        drawFloatingChars();

        if (!maskNeeded) {

            drawScore();

            // Same visibility as the player sprite itself (isPlayerHidden(), mellon.c, plus the
            // same exit-fade check drawPlayerSprite() (drawPlayer.c) makes locally rather than
            // through isPlayerHidden() -- that's teleport-only by design) -- otherwise a carried
            // item is left floating in place, with no player drawn under it, for as long as
            // drawPlayerSprite() is suppressing the sprite for a teleport departure/arrival or
            // an exit-sequence fade to black.
            //
            // Also suppressed once the door-exit drop (exitDepartOriginX/Y's attachmentOffset
            // arc, mellon.c's exit trigger) has run its course -- attachmentOffset naturally
            // exhausts back to 0 a couple of dozen real frames after landing (dropOffset[]'s
            // REST_HOLD, mellon.c), long before playerExitFade catches up to 15, and without
            // this the item fell back to its default "carried above head" draw offset for the
            // rest of the walk-off/fade -- popping back into view floating at the door, above
            // and behind the drifting-away player, instead of staying down where it was
            // dropped. Teleport doesn't need the equivalent check: isPlayerHidden() already
            // hides it within a frame or two of landing, well before its own arc could exhaust.

            if (!isPlayerHidden() && !(exitMode && playerExitFade >= 15) &&
                !(exitMode && attachment && !attachmentOffset))
                drawAttachedChar(attachment);

            drawMace();
            // drawRope();
            // drawGun();
            makeRain();
            drawParticles();
        }
    }

    interleaveChronoColour(&roller);
    adjustLuminance(0);

#if ENABLE_SWIPE
    applySwipeMask(_BUF_GAME_PF0_LEFT);    // must happen after everything else has drawn
#endif

    // scheduledTasks() used to be called here -- gets the MOST time of the two calls, since OS_Game's
    // own scheduledTasks() call below runs after drawScreen() eats most of that phase's budget. Now
    // dispatched generically by runARM_VerticalBlank() right after this function returns -- same slot.
}

void OS_Game() {

    // Deferred from movePlayer() (mellon.c), which sets this from deep inside a board scan --
    // switching caves involves setSchedule(SCHEDULE_UNPACK_CAVE) and wiping/redecoding the board
    // that same scan is still iterating over, so it can't safely happen right there. OS_Game() is
    // a clean top-level per-frame entry point (never itself called from inside a board scan), so
    // it's the first safe place to actually act on the request; the extra frame of latency this
    // costs is imperceptible against a multi-second "stand still" timer.
    if (teleportRequested) {
        teleportRequested = false;
        startTeleportWarp();
        return;    // this frame's cave/schedule state just changed under us -- nothing below applies
    }

    (*caveList[cave].handler)();

    // Also skipped once a transition is pending (gameState != nextGameState) --
    // drawScreen() alone costs ~78K of this phase's budget (see the comment
    // below), which can leave scheduleInitState() without the margin it needs
    // to ever complete. We're leaving this screen anyway once a transition is
    // armed, so there's nothing lost by not redrawing it that one last frame.
    if (gameSchedule != SCHEDULE_UNPACK_CAVE && gameState == nextGameState) {
        drawScreen();
        setPFColours(theCave->palette, (unsigned char *)(RAM + _BUF_GAME_COLUPF));


        drawPlayerSprite();    // MUST be in OS to avoid 1-frame lag in sprite pos
    }

    getJoystick();
    bufferedSWCHA &= swcha;    // | inhibitSWCHA;


    // scheduledTasks() used to be called here -- gets the LEAST time of the two calls, because
    // drawScreen() above has already used ~78K of this phase's budget. Now dispatched generically by
    // runARM_Overscan() right after this function returns -- same slot.
}

// EOF
