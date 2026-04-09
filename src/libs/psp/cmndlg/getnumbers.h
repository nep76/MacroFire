/*
	Common dialog - Get numbers
*/

#ifndef CMNDLG_GET_NUMBERS_H
#define CMNDLG_GET_NUMBERS_H

#include <stdlib.h>
#include "psp/blit.h"
#include "psp/ctrlpad.h"
#include "utils/strutil.h"
#define CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg.h"
#undef CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg/message.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define CMNDLG_GET_NUMBERS_MAX_DIGITS 9

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef struct {
	const char *title;
	const char *unit;
	long       *numSave;
	int        numDigits;
	int        selectedPlace;
} CmndlgGetNumbersData;

typedef struct {
	CmndlgGetNumbersData *data;
	int                  numberOfData;
	int                  selectDataNumber;
	CmndlgRc             rc;
	CmndlgUI             ui;
	CmndlgBaseParams     base; /* ダイアログが内部で使用。変更してはいけない。 */
} CmndlgGetNumbersParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
CmndlgGetNumbersParams *cmndlgGetNumbersGetParams( void );
CmndlgState cmndlgGetNumbersGetStatus( void );
int cmndlgGetNumbersStart( CmndlgGetNumbersParams *params );
int cmndlgGetNumbersUpdate( void );
int cmndlgGetNumbersShutdownStart( void );

#ifdef __cplusplus
}
#endif

#endif
