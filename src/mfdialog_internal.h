/*=========================================================

	mfdialog_internal.h

	ダイアログラッパー内部で使用。

=========================================================*/
#ifndef MFDIALOG_INTERNAL_H
#define MFDIALOG_INTERNAL_H

#include "macrofire.h"

/*=========================================================
	型宣言
=========================================================*/
#include "mfdialog.h"
#include "mfmenu.h"

/*=========================================================
	関数
=========================================================*/
inline void mfDialogInit( PadutilRemap *remap );
inline void mfDialogFinish( void );

bool mfDialogMessageDraw( void );
bool mfDialogMessageResult( void );

bool mfDialogSoskDraw( void );
bool mfDialogSoskResult( void );

bool mfDialogNumeditDraw( void );
bool mfDialogNumeditResult( void );

bool mfDialogGetfilenameDraw( void );
bool mfDialogGetfilenameResult( void );

bool mfDialogDetectbuttonsDraw( void );
bool mfDialogDetectbuttonsResult( void );

#endif
