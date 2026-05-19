#include "defines_dasm.h"

#include "cdfjplus.h"
#include "main.h"

void initKernel_Example() {

    setJumpVectors(_BUF_EX_JUMP, _kernelExample, _exampleExit, _SCANLINES);
    setPointer(DSJMP1PTR, _BUF_EX_JUMP);
}


void initialise_GS_Example() {

    frame = 0;
}

void VB_GS_CouchCompliant() {

    setPointer(DSJMP1PTR, _BUF_EXAMPLE_JUMP);

    // if (frame > 200)
    //     setGameState(GS_NEXT);

    return;
}

void OS_GS_Example() {
}

// EOF
