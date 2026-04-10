/*=========================================================

	cdialog/dev.h

	共通ダイアログ開発用。

=========================================================*/
#ifndef CDIALOG_DEV_H
#define CDIALOG_DEV_H

#include <pspkernel.h>
#include <psphprm.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "utils/strutil.h"
#include "psp/padctrl.h"
#include "psp/padutil.h"
#include "psp/gb.h"

#define CDIALOG_FUNCTION
#include "psp/cdialog.h"
#undef CDIALOG_FUNCTION

/*=========================================================
	マクロ
=========================================================*/
#define CDIALOG_VOID_ABORT( err, syserr ) { cdialogDevSetLastError( err, syserr ); return }
#define CDIALOG_ABORT( ret, err, syserr ) { cdialogDevSetLastError( err, syserr ); return ret }

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================
	型宣言
=========================================================*/
struct cdialog_dev_color {
	unsigned int fg;
	unsigned int bg;
	unsigned int fcfg;
	unsigned int fcbg;
	unsigned int border;
	unsigned int title;
	unsigned int help;
	unsigned int extra;
};

struct cdialog_dev_base_params {
	CdialogStatus status;
	CdialogResult result;
	unsigned short x, y;
	unsigned short width, height;
	struct cdialog_dev_color *color;
	PadctrlUID paduid;
	PadutilAnalogStick analogStick;
	PadutilRemap *remap;
};

/*=========================================================
	関数定義
=========================================================*/
bool cdialogDevLock( void );
bool cdialogDevUnlock( void );
void cdialogDevInitBaseParams( struct cdialog_dev_base_params *params );
int cdialogDevPrepareToStart( struct cdialog_dev_base_params *params, unsigned int options );
void cdialogDevPrepareToFinish( struct cdialog_dev_base_params *params );
void cdialogDevSetAnalogStickAdjust( struct cdialog_dev_base_params *params, PadutilCoord x, PadutilCoord y, PadutilCoord deadzone, PadutilSensitivity sens );
void cdialogDevSetRepeatButtons( struct cdialog_dev_base_params *params, unsigned int buttons );
PadutilButtons cdialogDevReadCtrlBuffer( struct cdialog_dev_base_params *params, SceCtrlData *pad, u32 *hprmkey );

#ifdef __cplusplus
}
#endif

#endif
