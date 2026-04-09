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

/*
	フックされたsceCtrlPeekBufferPositive()を内部で呼ぶフラグ。
	sceCtrlPeekBufferPositiveのシステムコールが発生し、その例外ハンドラとして呼ばれないと、
	オリジナルのAPI (本来のsceCtrlPeekBufferPositive) は0x80000023を返すっぽい。
	例外が発生していないのでCPUが特権モードになっていないから？
	PSP_MODULE_INFOのPSP_MODULE_KERNELは関係ないのだろうか？
	
	それを回避する力業として、普通はあり得ない値として第二引数に0となるMF_INTERNAL_CALLをセットする。
*/
#define MF_INTERNAL_CALL 0

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
