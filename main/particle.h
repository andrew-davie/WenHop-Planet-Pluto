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
};

struct Particle {

    unsigned char type;
    unsigned char age;
    unsigned char speed;
    unsigned char colour;
    int x;
    int y;
    unsigned char dir;
    unsigned short distance;
};

extern struct Particle particle[PARTICLE_COUNT];
extern const short sin_cos[32];
extern int weapon;

void initParticles();
void drawParticles();

void initTool();
void drawRope();
void drawMace();
void drawGun();


int sphereDot(int dotX, int dotY, int type, unsigned char age, unsigned char colour);
int nDots(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed,
          unsigned char colour);
void nDotsAtTrixel(int count, int dripX, int dripY, unsigned char age, enum ParticleType type, int speed,
                   unsigned char colour);

// EOF
