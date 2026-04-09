/*
	MacroFire header
*/

#ifndef __MACROFIRE__
#define __MACROFIRE__

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define GLOBAL_VARIABLES_DEFINE
#include "global.h"
#undef GLOBAL_VARIABLE_DEFINE

#include "hook.h"
#define MFTABLE_DEFINE
#include "mftable.h"
#undef MFTABLE_DEFINE

#include "menu.h"
#include "psp/blit.h"

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count );
int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count );
int mfCtrlReadBufferPositive( SceCtrlData *pad, int count );
int mfCtrlReadBufferNegative( SceCtrlData *pad, int count );
int mfCtrlPeekLatch( SceCtrlLatch *latch );
int mfCtrlReadLatch( SceCtrlLatch *latch );

int main_thread( SceSize arglen, void *argp );
int module_start( SceSize arglen, void *argp );
int module_stop( void );

#endif
