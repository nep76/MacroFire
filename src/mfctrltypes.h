/*=========================================================

	mfctrltypes.h

	共用する型宣言。

=========================================================*/
#ifndef MFCTRLTYPES_H
#define MFCTRLTYPES_H

#include "mfcommontypes.h"

/*=========================================================
	コントロールメッセージ
=========================================================*/
typedef enum {
	MF_CM_INIT = 0,
	MF_CM_LABEL,
	MF_CM_INFO,
	MF_CM_PROC,
	MF_CM_TERM,
	MF_CM_DIALOG_START,
	MF_CM_DIALOG_UPDATE,
	MF_CM_DIALOG_FINISH,
	MF_CM_EXTRA,
	MF_CM_SIGNAL,
} MfCtrlMessage;

/*=========================================================
	コントロールプロシージャ
=========================================================*/
typedef bool ( *MfControl )( MfCtrlMessage message, const char *label, void *var, void *arg, void *ex );

#endif
