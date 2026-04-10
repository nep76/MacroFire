/*=========================================================

	cdialog/message.h

	メッセージダイアログ。

=========================================================*/
#ifndef CDIALOG_MESSAGE_H
#define CDIALOG_MESSAGE_H

#include "dev.h"

/*=========================================================
	定数
=========================================================*/
#define CDIALOG_MESSAGE_TITLE_LENGTH 32
#define CDIALOG_MESSAGE_LENGTH       512

#define CDIALOG_MESSAGE_ANS_YESNO "[\x85]Yes  [\x86]No"
#define CDIALOG_MESSAGE_ANS_OK    "[\x85]OK"

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================
	型宣言
=========================================================*/
typedef enum {
	CDIALOG_MESSAGE_YESNO = 0x00000001,
} CdialogMessageOptions;

typedef struct {
	char title[CDIALOG_MESSAGE_TITLE_LENGTH];
	unsigned int options;
	char message[CDIALOG_MESSAGE_LENGTH];
} CdialogMessageData;

typedef struct {
	bool destroySelf;
	struct cdialog_dev_base_params base;
	CdialogMessageData data;
} CdialogMessageParams;

/*=========================================================
	関数
=========================================================*/
int cdialogMessageInit( CdialogMessageParams *params );
CdialogMessageData *cdialogMessageGetData( void );
CdialogStatus cdialogMessageGetStatus( void );
CdialogResult cdialogMessageGetResult( void );
int cdialogMessageStart( unsigned short x, unsigned short y );
int cdialogMessageStartNoLock( unsigned short x, unsigned short y );
int cdialogMessageUpdate( void );
int cdialogMessageShutdownStartNoLock( void );
int cdialogMessageShutdownStart( void );
void cdialogMessageDestroy( void );

#ifdef __cplusplus
}
#endif

#endif
