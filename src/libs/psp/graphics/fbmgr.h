/*
	Frame Buffer Manager
	
	簡易的なフレームバッファ管理。
	GUを使わずCPUのみでダブルバッファを行う。
*/

#ifndef SCR_WIDTH
#define SCR_WIDTH  480
#endif
#ifndef SCR_HEIGHT
#define SCR_HEIGHT 272
#endif
#ifndef BUF_WIDTH
#define BUF_WIDTH  512
#endif

#ifndef FBMGR_H
#define FBMGR_H

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <stdbool.h>
#include <string.h>
#include "psp/memsce.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define FBMGR_MEM_NOCACHE 0x40000000

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef struct {
	int mode;
	int width;
	int height;
	int bufferWidth;
	enum PspDisplayPixelFormats pixelFormat;
	void *frameBuffer;
} FbmgrDisplayStat;

/*-----------------------------------------------
	関数
-----------------------------------------------*/

/*
	フレームバッファマネージャの初期化。
	ダブルバッファの準備を行う。
	1枚目のフレームバッファには、現在の表示バッファを再利用する。
	2枚目のフレームバッファは引数でメモリアドレスを指定する。
	
	@param void *fbp
		2枚目のフレームバッファとなるメモリアドレスを渡す。
		NULLを渡すと、自動的に必要なサイズをユーザメモリから確保する。
	
	@return void*
		次のフレームバッファ(描画バッファ)のアドレス。
*/
void *fbmgrInit( void *fbp );

/*
	フレームバッファマネージャの破棄と後片付け。
	ディスプレイモードをfbmgrInit()が呼ばれる前の状態に戻す。
	更にfbmgrInit()で引数にNULLを渡していた場合は、内部で確保したメモリを解放する。
*/
void fbmgrDestroy( void );

/*
	表示バッファと描画バッファを切り替える。
	イメージ的にはsceGuSwapBuffers()のやっていることと同じ。
	
	切り替えた後、新しい描画バッファは前回のデータが残っているので、
	自分で再利用するなりクリアするなりすること。
	
	@return void*
		次のフレームバッファ(描画バッファ)のアドレス。
*/
void *fbmgrSwapBuffers( void );

/*
	表示バッファへ描画バッファの内容をコピーする。
	もう使わない。
*/
void fbmgrBlitBuffers( void );

/*
	現在の表示バッファのアドレスを返す。
	
	もし、fbmgrInit()に引数を渡してフレームバッファを指定していても、
	それらアドレスには必ず0x40000000が加算されている。
	これはデータキャッシュを無効化するフラグ。
*/
void *fbmgrGetCurrentDispBuf( void );

/*
	現在の描画バッファのアドレスを返す。
	
	もし、fbmgrInit()に引数を渡してフレームバッファを指定していても、
	それらアドレスには必ず0x40000000が加算されている。
	これはデータキャッシュを無効化するフラグ。
*/
void *fbmgrGetCurrentDrawBuf( void );

/*
	内部フレームバッファのアドレスを返す。
	fbmgrInit()の引数にNULLを渡した場合に、自動確保されるメモリアドレスを返す。
	fbmgrInit()に引数を渡していた場合は常にNULLが返る。
	
	@return void*
		内部で確保したフレームバッファのアドレス。
*/
void *fbmgrGetInternalFrameBufAddr( void );

/*
	現在のディスプレイモードとフレームバッファ情報を取得する。
	
	@param FbmgrDisplayStat *stat
		情報がセットされる構造体。
	
	@return int
		0で成功。
*/
int fbmgrGetDisplayStat( FbmgrDisplayStat *stat );

/*
	ピクセルフォーマットごとのバイト数を返す。
	
	@param enum PspDisplayPixelFormats pxfmt
		ピクセルフォーマット。
	
	@return size_t
		バイト数。
*/
size_t fbmgrGetPixelLength( enum PspDisplayPixelFormats pxfmt );

#ifdef __cplusplus
}
#endif

#endif
