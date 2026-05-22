#pragma once

#include "defines_dasm.h"

extern void (*const initialiseKernel[_KERNEL_MAX])();

void initKernel_Copyright();
void initKernel_Rainbow();
void initKernel_DetectConsole();
void initKernel_CouchCompliant();
void initKernel_Menu();
void initKernel_Game();

// EOF
