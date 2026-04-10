/*=========================================================

	mfanalogstick.c

	アナログスティックの調整。

=========================================================*/
#ifndef MFANALOGSTICK_H
#define MFANALOGSTICK_H

#include "macrofire.h"

/*=========================================================
	関数
=========================================================*/
void mfAnalogStickInit( void );
void mfAnalogStickIniLoad( IniUID ini, char *buf, size_t len );
void mfAnalogStickIniSave( IniUID ini, char *buf, size_t len );
bool mfAnalogStickIsEnabled( void );
void mfAnalogStickAdjust( SceCtrlData *pad );
void mfAnalogStickMenu( MfMessage message );
const PadutilAnalogStick *mfAnalogStickGetContext( void );

#endif
