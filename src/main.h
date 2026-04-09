/*
	MacroFire header
*/

#ifndef MFMAIN_H
#define MFMAIN_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define GLOBAL_VARIABLES_DEFINE
#include "macrofire.h"
#undef GLOBAL_VARIABLE_DEFINE

#include "hook.h"
#define MFTABLE_DEFINE
#include "mftable.h"
#undef MFTABLE_DEFINE

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/

/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef int ( *MfSceCtrlDataFunc  )( SceCtrlData*, int );
typedef int ( *MfSceCtrlLatchFunc )( SceCtrlLatch* );

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count );
int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count );
int mfCtrlReadBufferPositive( SceCtrlData *pad, int count );
int mfCtrlReadBufferNegative( SceCtrlData *pad, int count );
int mfCtrlPeekLatch( SceCtrlLatch *latch );
int mfCtrlReadLatch( SceCtrlLatch *latch );

int main_thread( SceSize arglen, void *argp );
int module_start( SceSize arglen, void *argp );
int module_stop( SceSize arglen, void *argp );

#include "hooktable.h"

#endif
