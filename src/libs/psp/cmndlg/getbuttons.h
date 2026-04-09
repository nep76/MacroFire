/*
	Common dialog - Get buttons
*/

#ifndef CMNDLG_GET_BUTTONS_H
#define CMNDLG_GET_BUTTONS_H

#include "psp/gb.h"
#include "psp/ctrlpad.h"
#include "psp/memsce.h"
#include "utils/strutil.h"
#define CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg.h"
#undef CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg/message.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
struct cmndlg_get_buttons_buttonstat {
	char *name;
	unsigned int button;
	bool available;
};

struct cmndlg_get_buttons_tempdata {
	unsigned int buttons;
	unsigned int selected;
};

typedef struct {
	const char   *title;
	unsigned int *buttonsSave;
	unsigned int buttonsAvailable;
} CmndlgGetButtonsData;

typedef struct {
	CmndlgGetButtonsData *data;
	int                  numberOfData;
	int                  selectDataNumber;
	CmndlgRc             rc;
	CmndlgUI             ui;
	CmndlgBaseParams     base; /* ダイアログが内部で使用。変更してはいけない。 */
} CmndlgGetButtonsParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
CmndlgGetButtonsParams *cmndlgGetButtonsGetParams( void );
CmndlgState cmndlgGetButtonsGetStatus( void );
int cmndlgGetButtonsStart( CmndlgGetButtonsParams *params );
int cmndlgGetButtonsUpdate( void );
int cmndlgGetButtonsShutdownStart( void );

#ifdef __cplusplus
}
#endif

#endif
