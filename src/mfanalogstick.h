/*=========================================================

	mfanalogstick.c

	アナログスティックの調整。

=========================================================*/
#ifndef MFANALOGSTICK_H
#define MFANALOGSTICK_H

#define MFEXCLUDE_ANALOGSTICK
#include "macrofire.h"
#undef MFEXCLUDE_ANALOGSTICK

/*=========================================================
	関数
=========================================================*/
void *mfAnalogStickProc( MfMessage message );
void mfAnalogStickIniLoad( IniUID ini, char *buf, size_t len );
void mfAnalogStickIniSave( IniUID ini, char *buf, size_t len );
void mfAnalogStickAdjust( MfHookAction action, SceCtrlData *pad );
void mfAnalogStickMenu( MfMessage message );
const PadutilAnalogStick *mfAnalogStickGetContext( void );

#endif
