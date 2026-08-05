#pragma once

#include <stdbool.h>

#define PARTICLE_COUNT 42
#define ROPE_PARTICLE_COUNT 42
#define PARTICLE_SPIRAL_ANGULAR_SPEED 6

enum ParticleType {

    PT_ONE = 1,
    PT_TWO,
    PT_SPIRAL,
    PT_SPIRAL2,
    PT_BUBBLE,
    PT_STATIC,
    PT_CHARACTER,
    PT_RAIN,
};

struct Particle {

    unsigned char type;
    unsigned char age;
    unsigned char speed;
    unsigned char colour;
    int trixX_8;
    int trixY_8;
    unsigned char dir;
    unsigned short distance;

    // PT_RAIN only: the rock/geodoge roll-off steering still runs normally (rolls sideways off a
    // rock/geodoge exactly like real rain when there's a blank pixel to roll onto), but every
    // point that would otherwise splash-and-terminate (water, board bounds, the player, no clear
    // side to roll toward, roll blocked with nowhere to go, any other solid pixel) instead just
    // lets it keep falling straight through -- no SFX_DRIP2, no splash burst, no termination --
    // until its own preset age runs out and it dies quietly. For a short decorative fall that
    // isn't real weather and, unlike real rain, typically spawns already touching/inside solid
    // background (e.g. right at an object's base) where real rain's collisions would otherwise
    // end it almost immediately. False (collides and splashes normally, unchanged) for every
    // other spawn, including real weather (makeRain()).
    bool silent;
};

extern struct Particle particle[PARTICLE_COUNT];
extern const short sin_cos[32];
extern int weapon;

void initParticles();
void zapNonSpiralParticles();
void drawParticles();
void makeRain();
void initWeather();

// theCave->weather == 255 means "storm builds over the level" -- see
// initWeather()/makeRain() in particle.c. weatherIntensity is the live,
// ramping frequency divisor makeRain() actually uses; theCave->weather
// itself never changes at runtime.
extern int weatherIntensity;

// True while a level-255 rainstorm is actively rising/peaking/falling;
// false during the dry gap between storms (or for non-255 weather).
int isStormActive();

void initTool();
void drawRope();
void drawMace();
void drawGun();


int sphereDot(int trixX, int trixY, int type, unsigned char age, unsigned char colour);
int nDots(int count, int trixX, int trixY, int type, unsigned char age, int offsetX, int offsetY, int speed,
          unsigned char colour);
void nDotsAtTrixel(int count, int trixX, int trixY, unsigned char age, enum ParticleType type, int speed,
                   unsigned char colour);

void floatingCharacter(int trixX, int trixY, int age, unsigned char ch);
void removeFloatingChars();
void drawFloatingChars();


// EOF
