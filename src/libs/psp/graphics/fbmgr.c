/*
	fbmgr.c
*/

#include "fbmgr.h"
	
/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static FbmgrDisplayStat st_save_stat;
static FbmgrDisplayStat st_stat;
static size_t           st_buflen;
static void             *st_disp_buf;
static void             *st_draw_buf;
static void             *st_internal_framebuf;

/*=============================================*/

void *fbmgrInit( void *draw_buf )
{
	/* 現在のディスプレイモードと表示バッファを取得 */
	fbmgrGetDisplayStat( &st_save_stat );
	
	/* これを複製し作業用を作成 */
	st_stat   = st_save_stat;
	
	/* ディスプレイモードからフレームバッファの容量を計算 */
	st_buflen = fbmgrGetPixelLength( st_stat.pixelFormat ) * st_stat.bufferWidth * st_stat.height;
	
	/*
		現在の表示バッファを1枚目のフレームバッファとして再利用
		(FBMGR_MEM_NOCACHEを加えてデータキャッシュを無効化)
	*/
	st_disp_buf = (void *)( (unsigned int)(st_stat.frameBuffer) | FBMGR_MEM_NOCACHE );
	
	if( ! draw_buf ){
		/* draw_bufが渡されていなければ、内部的にユーザメモリから確保 */
		st_internal_framebuf = memsceMalloc( st_buflen );
		
		if( ! st_internal_framebuf ) return NULL;
		
		/* FBMGR_MEM_NOCACHE(データキャッシュ無効化)フラグを加える */
		st_draw_buf = (void *)( (unsigned int)st_internal_framebuf | FBMGR_MEM_NOCACHE );
	} else{
		/* FBMGR_MEM_NOCACHE(データキャッシュ無効化)フラグを加える */
		st_draw_buf = (void *)( (unsigned int)draw_buf | FBMGR_MEM_NOCACHE );
	}
	
	return st_draw_buf;
}

size_t fbmgrFrameBufSize( void )
{
	return st_buflen;
}

void fbmgrDestroy( void )
{
	/* フレームバッファのアドレスを初期化前の位置に戻す */
	sceDisplaySetFrameBuf( st_save_stat.frameBuffer, st_save_stat.bufferWidth, st_save_stat.pixelFormat, PSP_DISPLAY_SETBUF_IMMEDIATE );
	
	/* 内部バッファをライブラリが自前確保していれば解放する */
	if( st_internal_framebuf ){
		memsceFree( (void *)( (unsigned int)st_internal_framebuf ^ FBMGR_MEM_NOCACHE ) );
		st_internal_framebuf = NULL;
	}
}

void *fbmgrGetCurrentDispBuf( void )
{
	return st_disp_buf;
}

void *fbmgrGetCurrentDrawBuf( void )
{
	return st_draw_buf;
}

void *fbmgrGetInternalFrameBufAddr( void )
{
	return st_internal_framebuf;
}

void *fbmgrSwapBuffers( void )
{
	void *swap  = st_disp_buf;
	st_disp_buf = st_draw_buf;
	st_draw_buf = swap;
	
	/* 新しい表示バッファを表示 */
	sceDisplaySetFrameBuf( st_disp_buf, st_stat.bufferWidth, st_stat.pixelFormat, PSP_DISPLAY_SETBUF_IMMEDIATE );
	
	return st_draw_buf;
}

void fbmgrBlitBuffers( void )
{
	memcpy( st_disp_buf, st_draw_buf, st_buflen );
}

int fbmgrGetDisplayStat( FbmgrDisplayStat *stat )
{
	int rval;
	
	rval = sceDisplayGetMode( &(stat->mode), &(stat->width), &(stat->height) );
	if( rval ) return rval;
	
	rval = sceDisplayGetFrameBuf( (void *)&(stat->frameBuffer), &(stat->bufferWidth), (int *)&(stat->pixelFormat), PSP_DISPLAY_SETBUF_IMMEDIATE );
	if( rval ) return rval;
	
	return 0;
}

size_t fbmgrGetPixelLength( enum PspDisplayPixelFormats pxfmt )
{
	switch( pxfmt ){
		case PSP_DISPLAY_PIXEL_FORMAT_565:
		case PSP_DISPLAY_PIXEL_FORMAT_5551:
		case PSP_DISPLAY_PIXEL_FORMAT_4444:
			return 2;
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
		default:
			return 4;
	}
}
