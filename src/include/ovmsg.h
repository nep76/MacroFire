/*=========================================================
	
	ovmsg.h
	
	オーバーレイメッセージ。
	
=========================================================*/
#ifndef OVMSG_H
#define OVMSG_H

#include <pspkernel.h>
#include <stdarg.h>
#include <stdbool.h>

/*=========================================================
	マクロ
=========================================================*/
#define OVMSG_MESSAGE_MAX_LENGTH 128

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================
	型宣言
=========================================================*/
typedef intptr_t OvmsgUID;

/*=========================================================
	関数
=========================================================*/
OvmsgUID ovmsgInit( void );
SceUID ovmsgGetThreadId( OvmsgUID uid );
void ovmsgSetColor( OvmsgUID uid, unsigned int fg, unsigned int bg );
void ovmsgSetPosition( OvmsgUID uid, unsigned short x, unsigned short y );
int ovmsgMain( SceSize arglen, void *argp );
int ovmsgVprintf( OvmsgUID uid, const char *format, va_list ap );
int ovmsgPrintf( OvmsgUID uid, const char *format, ... );
void ovmsgSuspend( OvmsgUID uid );
void ovmsgShutdown( OvmsgUID uid );
void ovmsgDestroy( OvmsgUID uid );

#ifdef __cplusplus
}
#endif

#endif
