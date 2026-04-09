/*
	Configration Manager
	
	設定項目ラベルについて。
		ファイルから読み込んだラベルについては空白文字の削除と全て大文字かによる
		ノーマライズを行うが、ConfmgrHandlerParamsのlabelメンバにはなにもしない。
		labelメンバの値を間違って書かないように注意。
*/

#ifndef __CONFMGR_H__
#define __CONFMGR_H__

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "utils/strutil.h"
#include "psp/fileh.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define CONFMGR_LABEL_BUFFER 64
#define CONFMGR_LINE_BUFFER  256

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef void ( *ConfmgrStoreCallback )( char buf[], size_t max,    void *mparam, void *sparam );
typedef void ( *ConfmgrLoadCallback  )( char *name, char   *value, void *mparam, void *sparam );

typedef enum {
	CH_LONGINT,
	CH_STRING,
	CH_CALLBACK
} ConfmgrHandleType;

typedef struct {
	/* 設定 */
	char *label;
	
	/* 設定タイプ */
	ConfmgrHandleType type;
	
	/* メインパラメータ(意味は設定タイプにより変化) */
	void *mparam;
	
	/* サブパラメータ(意味は設定タイプにより変化) */
	void *sparam;
	
	/* 設定タイプCH_CALLBACK時のみ使用。confmgrLoadからコールバックされる関数。 */
	ConfmgrLoadCallback  handlerLoad;
	
	/* 設定タイプCH_CALLBACK時のみ使用。confmgrStoreからコールバックされる関数。 */
	ConfmgrStoreCallback handlerStore;
} ConfmgrHandler;

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/
int confmgrStore( ConfmgrHandler *handlers, int count, const char *fullpath );
int confmgrLoad( ConfmgrHandler *handlers, int count, const char *fullpath );

#ifdef __cplusplus
}
#endif

#endif
