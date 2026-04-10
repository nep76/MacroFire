/*
	Common dialog - Simplified On Screen Keyboard
*/

#ifndef SOSK_H
#define SOSK_H

#include "psp/gb.h"
#include "psp/ctrlpad.h"
#include "psp/memsce.h"
#include "utils/strutil.h"
#define CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg.h"
#undef CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg/message.h"


/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define CMNDLG_SOSK_ROWS         8
#define CMNDLG_SOSK_COLS         13
#define CMNDLG_SOSK_WIDTH        ( CMNDLG_SOSK_COLS * 2 + 12 )
#define CMNDLG_SOSK_HEIGHT       ( CMNDLG_SOSK_ROWS * 2 + 6  )
#define CMNDLG_SOSK_RESULT_WIDTH ( CMNDLG_SOSK_WIDTH - 6 )

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/

enum cmndlg_sosk_options {
	CMNDLG_SOSK_DISPLAY_CENTER = 0x00000001
};

struct cmndlg_sosk_tempdata {
	char *workText;
	int  currentLength;
	int  cursorPos;
	int  textOffset;
	int  cx, cy;
};

typedef struct {
	const char       *title;
	char             *text;
	size_t           textMax;
	unsigned int     options;
	CmndlgRc         rc;
	CmndlgUI         ui;
	CmndlgBaseParams base; /* 内部で使用。何もしなくてよい。 */
} CmndlgSoskParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
CmndlgSoskParams *cmndlgSoskGetParams( void );
CmndlgState cmndlgSoskGetStatus( void );
int cmndlgSoskStart( CmndlgSoskParams *params );
int cmndlgSoskUpdate( void );
int cmndlgSoskShutdownStart( void );

#ifdef __cplusplus
}
#endif

#endif
