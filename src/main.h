/*
	MacroFire header
*/

#ifndef MACROFIRE
#define MACROFIRE

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

#include "utils/inimgr.h"
#include "menu.h"
#include "psp/blit.h"

/*-----------------------------------------------
	íËêî
-----------------------------------------------*/
#define MF_CONF_NUM             3
#define MF_CONF_FILE_FULLPATH   "ms0:/seplugins/macrofire_conf.txt"
#define MF_DEFAULT_MENU_BUTTONS ( PSP_CTRL_VOLDOWN | PSP_CTRL_VOLUP )
#define MF_ENGINE_STARTUP_ON    "ON"
#define MF_ENGINE_STARTUP_OFF   "OFF"

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

#endif
