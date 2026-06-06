#pragma once

#define PARTICLE_COUNT 24
#define ROPE_PARTICLE_COUNT 15
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
    unsigned char direction;
    unsigned short distance;
};

extern struct Particle particle[PARTICLE_COUNT];

void initParticles();
void drawParticles();
void drawRope();
void drawMace();


int sphereDot(int dotX, int dotY, int type, unsigned char age, unsigned char colour);
void nDots(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed,
           unsigned char colour);
void nDotsBackwards(int count, int dripX, int dripY, int type, unsigned char age, int offsetX, int offsetY, int speed);
void nDotsAtTrixel(int count, int dripX, int dripY, unsigned char age, int speed, unsigned char colour);

// EOF
