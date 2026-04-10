/*=========================================================

	mfmain.h

	MacroFire メインループ

=========================================================*/
#ifndef MFMAIN_H
#define MFMAIN_H

#define MF_FIRST_INCLUDE
#include "macrofire.h"
#undef MF_FIRST_INCLUDE

/*=========================================================
	デフォルト設定値
=========================================================*/
#define MF_INI_STARTUP             false
#define MF_INI_MENU_BUTTONS        ( PSP_CTRL_VOLUP | PSP_CTRL_VOLDOWN )
#define MF_INI_TOGGLE_BUTTONS      ( 0 )
#define MF_INI_STATUS_NOTIFICATION false

/*=========================================================
	プロトタイプ
=========================================================*/
int mfVshctrlReadBufferPositive( SceCtrlData *pad, int count );
int mfCtrlReadBufferPositive( SceCtrlData *pad, int count );
int mfCtrlReadBufferNegative( SceCtrlData *pad, int count );
int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count );
int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count );
int mfCtrlPeekLatch( SceCtrlLatch *latch );
int mfCtrlReadLatch( SceCtrlLatch *latch );
int mfHprmPeekCurrentKey( u32 *key );
int mfHprmPeekLatch( u32 *latch );
int mfHprmReadLatch( u32 *latch );
int mfImposeHomeButton( int unk );

/*=======================================================*/

#include "hooktable.h"

#endif
