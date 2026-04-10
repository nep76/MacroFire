/*=========================================================

	mfdialog.h

	ダイアログラッパー

=========================================================*/
#ifndef MFDIALOG_H
#define MFDIALOG_H

#include "macrofire.h"
#include "cdialog.h"

/*=========================================================
	型宣言
=========================================================*/
typedef enum {
	MF_DIALOG_NONE = 0,
	MF_DIALOG_MESSAGE,
	MF_DIALOG_SOSK,
	MF_DIALOG_NUMEDIT,
	MF_DIALOG_GETFILENAME,
	MF_DIALOG_DETECTBUTTONS
} MfDialogType;

/*=========================================================
	関数
=========================================================*/
inline MfDialogType mfDialogCurrentType( void );
inline bool mfDialogLastResult( void );

bool mfDialogMessage( const char *title, const char *message, unsigned int error, bool yesno );

bool mfDialogSosk( const char *title, char *text, size_t length, unsigned int availkb );

bool mfDialogNumedit( const char *title, const char *unit, void *num, uint32_t max );

bool mfDialogGetfilename( const char *title, const char *initdir, const char *initname, char *path, size_t pathmax, unsigned int options );

bool mfDialogDetectbuttons( const char *title, PadutilButtons availbtns, PadutilButtons *buttons );

#endif
