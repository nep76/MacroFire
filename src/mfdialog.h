/*=========================================================

	mfdialog.h

	ダイアログラッパー

=========================================================*/
#ifndef MFDIALOG_H
#define MFDIALOG_H

#define MFEXCLUDE_MENU
#define MFEXCLUDE_DIALOG
#include "macrofire.h"
#undef MFEXCLUDE_MENU
#undef MFEXCLUDE_DIALOG

#include "psp/cdialog.h"
#include "utils/strutil.h"

/*=========================================================
	型宣言
=========================================================*/
typedef enum {
	MFDIALOG_NONE = 0,
	MFDIALOG_MESSAGE,
	MFDIALOG_SOSK,
	MFDIALOG_NUMEDIT,
	MFDIALOG_GETFILENAME,
	MFDIALOG_DETECTBUTTONS
} MfDialogType;

#include "mfmenu.h"

/*=========================================================
	関数
=========================================================*/
inline void mfDialogInit( PadutilRemap *remap );
inline MfDialogType mfDialogCurrentType( void );
inline void mfDialogFinish( void );

bool mfDialogMessageInit( const char *title, const char *message, bool yesno );
bool mfDialogMessageDraw( void );
bool mfDialogMessageResult( void );

bool mfDialogSoskInit( const char *title, char *text, size_t length, unsigned int availkb );
bool mfDialogSoskDraw( void );
bool mfDialogSoskResult( void );

bool mfDialogNumeditInit( const char *title, const char *unit, void *num, uint32_t max );
bool mfDialogNumeditDraw( void );
bool mfDialogNumeditResult( void );

bool mfDialogGetfilenameInit( const char *title, const char *initdir, const char *initname, char *path, size_t pathmax, unsigned int options );
bool mfDialogGetfilenameDraw( void );
bool mfDialogGetfilenameResult( void );

bool mfDialogDetectbuttonsInit( const char *title, PadutilButtons availbtns, PadutilButtons *buttons );
bool mfDialogDetectbuttonsDraw( void );
bool mfDialogDetectbuttonsResult( void );

#endif
