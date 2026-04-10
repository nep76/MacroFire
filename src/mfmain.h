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
	定数
=========================================================*/
#define MF_GAMEID_ADDRESS 0x88013670

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
int mfHprmPeekCurrentKey( u32 *key );
int mfHprmPeekLatch( u32 *latch );
int mfHprmReadLatch( u32 *latch );
void mfSysconSetDebugHandlers( SceSysconDebugHandlers *handlers );

/*=======================================================*/

#include "hooktable.h"

#endif
