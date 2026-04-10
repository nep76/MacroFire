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

#define MFTABLE_DEFINE
#include "mftable.h"
#undef MFTABLE_DEFINE

#include "psp/hook.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define MF_INIDEF_MAIN_STARTUP        false
#define MF_INIDEF_MAIN_MENUBUTTONS    ( PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN )
#define MF_INIDEF_MAIN_TOGGLEBUTTONS  ( 0 )
#define MF_INIDEF_ANALOGTUNE_ORIGINX  128
#define MF_INIDEF_ANALOGTUNE_ORIGINY  128
#define MF_INIDEF_ANALOGTUNE_DEADZONE 40

/*-----------------------------------------------
	å^êÈåæ
-----------------------------------------------*/
typedef int ( *MfSceCtrlDataFunc  )( SceCtrlData*, int );
typedef int ( *MfSceCtrlLatchFunc )( SceCtrlLatch* );

enum mf_interrupt_handler_mode {
	MF_IHM_USER = 0,
	MF_IHM_KERNEL
};

/*-----------------------------------------------
	ä÷êî
-----------------------------------------------*/
int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count );
int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count );
int mfCtrlReadBufferPositive( SceCtrlData *pad, int count );
int mfCtrlReadBufferNegative( SceCtrlData *pad, int count );
int mfCtrlPeekLatch( SceCtrlLatch *latch );
int mfCtrlReadLatch( SceCtrlLatch *latch );
int mfVshCtrlReadBufferPositive( SceCtrlData *pad, int count );

int main_thread( SceSize arglen, void *argp );
int module_start( SceSize arglen, void *argp );
int module_stop( SceSize arglen, void *argp );

#include "hooktable.h"

#endif
