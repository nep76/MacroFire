/*==========================================================

	pspapi.h

	PSPSDKにプロトタイプがないAPI。

==========================================================*/
#ifndef PSPAPI_H
#define PSPAPI_H

#include <pspkernel.h>

/*-----------------------------------------------
	sceDisplayEnable
	sceDisplayDisable
	
	LCDの電源のON/OFFを切り替えるようだ？
-----------------------------------------------*/
void sceDisplayEnable( void );
//void sceDisplayDisable( void );

/*-----------------------------------------------
	sceDmacMemcpy
	sceDmacTryMemcpy (async(?))
	
	背景のような大きなメモリブロックの転送にはDMAコントローラを使用。
	細切れのデータに対しては逆に遅くなるとの情報があったので背景転送にのみ使う。
	本当に遅いのかどうかは調べていない。
	
	キャッシュの関係で摩訶不思議なバグに悩まれることがあるっぽい。
	通常はmemcpy()を使い、データが多少化けても問題のない背景転送などに使った方が無難？
-----------------------------------------------*/
int  sceDmacMemcpy( void *dest, const void *src, SceSize size );
//int  sceDmacTryMemcpy( void *dest, const void *src, SceSize size );

/*-----------------------------------------------
	sceSysregAudioIoEnable
	sceSysregAudioIoDisable
	
	オーディオ入出力の有効化/無効化。
	どちらもpspSdkSetK1( 0 )実行後でないと不思議な動作をする。
-----------------------------------------------*/
int sceSysregAudioIoEnable( int unk );
int sceSysregAudioIoDisable( int unk );

#endif
