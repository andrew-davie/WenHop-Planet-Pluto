#include "defines_dasm.h"

#include "cdfjplus.h"

#include "joystick.h"
#include "main.h"

void getJoystick() {
    swcha = RAM[_SWCHA];
    inpt4 = RAM[_INPT4];
}

// EOF
