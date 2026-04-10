/*=========================================================

	mfmenutypes.h

	メニュー用型宣言。

=========================================================*/
#ifndef MFMENUTYPES_H
#define MFMENUTYPES_H

#include "mfctrltypes.h"

/*=========================================================
	マクロ
=========================================================*/
#define mfMenuIsPressed( buttons )     ( ( mfMenuGetCurrentButtons() & ( buttons ) ) == ( buttons ) ? true : false )
#define mfMenuIsOnlyPressed( buttons ) ( mfMenuGetCurrentButtons() == ( buttons ) ? true : false )
#define mfMenuIsAnyPressed( buttons )  ( ( mfMenuGetCurrentButtons() & ( buttons ) ) ? true : false )

#define MF_MESSAGE_DELAY 1000000
#define MF_ERROR_DELAY   3000000

/*=========================================================
	型宣言
=========================================================*/
typedef enum {
	MF_MM_INIT = 0,
	MF_MM_PROC,
	MF_MM_TERM,
	MF_MM_EXTRA
} MfMenuMessage;

typedef void ( *MfMenuProc )( MfMenuMessage );

typedef struct {
	bool      active;
	MfControl ctrl;
	char      *label;
	void      *var;
	void      *arg;
} MfMenuEntry;

typedef struct {
	char *label;
	int x, y;
	unsigned short rows, cols;
	unsigned int   *colsWidth;
	MfMenuEntry    **entry;
} MfMenuTable;

#endif
