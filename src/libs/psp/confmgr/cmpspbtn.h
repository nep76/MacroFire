/*
	Configuration Manager用ボタン取得コールバック関数
*/

#ifndef __CONFMGR_BUTTONS_H__
#define __CONFMGR_BUTTONS_H__

#include <pspctrl.h>
#include <string.h>
#include "utils/strutil.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	宣言
-----------------------------------------------*/
struct confmgr_pspbtn {
	unsigned int button;
	char *label;
};

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/
void cmpspbtnSetMask( unsigned int mask );
void cmpspbtnLoad( char *name, char *value, void *mparam, void *sparam );
void cmpspbtnStore( char buf[], size_t max, void *mparam, void *sparam );

#ifdef __cplusplus
}
#endif

#endif
