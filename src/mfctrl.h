/*=========================================================

	mfctrl.h

	組み込みコントロール。

=========================================================*/
#ifndef MFCTRL_H
#define MFCTRL_H

#define MFEXCLUDE_CTRL
#include "macrofire.h"
#undef MFEXCLUDE_CTRL

/*=========================================================
	マクロ
=========================================================*/
#define MF_CTRL_BUFFER_LENGTH 250

/*=========================================================
	設定構造体
=========================================================*/
typedef struct {
	char *unit;
	unsigned int min, max, width;
} MfCtrlDefGetNumberPref;

typedef struct {
	PadutilButtons availButtons;
} MfCtrlDefGetButtonsPref;
/*
typedef struct {
	char *initDir;
	char *initFilename;
	size_t pathMax
	unsigned int options;
} MfCtrlDefGetFilenamePref;
*/
/*=========================================================
	関数定義
=========================================================*/
void mfCtrlSetLabel( void *arg, char *format, ... );
bool mfCtrlDefBool( MfMessage message, const char *label, void *var, void *arg, void *ex );
bool mfCtrlDefOptions( MfMessage message, const char *label, void *var, void *arg, void *ex );
bool mfCtrlDefCallback( MfMessage message, const char *label, void *func, void *arg, void *ex );
bool mfCtrlDefExtra( MfMessage message, const char *label, void *func, void *arg, void *ex );
bool mfCtrlDefGetButtons( MfMessage message, const char *label, void *var, void *pref, void *ex );
bool mfCtrlDefGetNumber( MfMessage message, const char *label, void *var, void *pref, void *ex );
unsigned int mfCtrlVarGetNum( void *num, unsigned int max );
void mfCtrlVarSetNum( void *num, unsigned int max, unsigned int value );
/*bool mfCtrlDefGetFilename( MfMessage message, const char *label, void *var, void *pref, void *ex );*/

#endif
