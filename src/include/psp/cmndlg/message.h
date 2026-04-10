/*
	Common dialog - Message
*/

#ifndef MESSAGE_H
#define MESSAGE_H

#include "psp/gb.h"
#include "psp/ctrlpad.h"
#define CMNDLG_FUNCTION_EXPORT
#include "psp/cmndlg.h"
#undef CMNDLG_FUNCTION_EXPORT

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define CMNDLG_MESSAGE_MAX_WIDTH    80
#define CMNDLG_MESSAGE_MAX_HEIGHT   34

#define CMNDLG_MESSAGE_PROMPT_YESNO "[\x85]YES  [\x86]NO"
#define CMNDLG_MESSAGE_PROMPT_OK    "[\x85]OK"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef enum {
	CMNDLG_MESSAGE_YESNO          = 0x00000001,
	CMNDLG_MESSAGE_DISPLAY_CENTER = 0x00000002
} CmndlgMessageOptions;

typedef struct {
	char                 title[64];
	char                 message[512];
	CmndlgMessageOptions options;
	CmndlgRc             rc;
	CmndlgUI             ui;
	CmndlgBaseParams     base; /* 内部で使用。何もしなくてよい。 */
} CmndlgMessageParams;

/*-----------------------------------------------
	関数
-----------------------------------------------*/
CmndlgMessageParams *cmndlgMessageGetParams( void );
CmndlgState cmndlgMessageGetStatus( void );
int cmndlgMessageStart( CmndlgMessageParams *params );
int cmndlgMessageUpdate( void );
int cmndlgMessageShutdownStart( void );

#ifdef __cplusplus
}
#endif

#endif
