#pragma once

#include "defines_dasm.h"

extern void (*const initialiseKernel[_KERNEL_MAX])();

extern void initKernel_Copyright();
extern void initKernel_Rainbow();
extern void initKernel_DetectConsole();
extern void initKernel_CouchCompliant();


// EOF
