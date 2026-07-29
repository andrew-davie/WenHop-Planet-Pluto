#pragma once

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
};

extern struct Particle particle[PARTICLE_COUNT];
extern const short sin_cos[32];
extern int weapon;

void initParticles();
void drawParticles();
void makeRain();

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
