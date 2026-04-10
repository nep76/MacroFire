/*=========================================================

	cdialog/message.c

	メッセージダイアログ。

=========================================================*/
#include "message.h"

/*=========================================================
	ローカル型宣言
=========================================================*/

/*=========================================================
	ローカル関数
=========================================================*/
static void cdialog_message_count_width_and_height( const char *str, unsigned short *width, unsigned short *height );
static void cdialog_message_draw( struct cdialog_dev_base_params *base, CdialogMessageData *data );

/*=========================================================
	ローカル変数
=========================================================*/
static CdialogMessageParams *st_params;

/*=========================================================
	関数
=========================================================*/
int cdialogMessageInit( CdialogMessageParams *params )
{
	if( st_params ) return CG_ERROR_ALREADY_INITIALIZED;
	
	if( params ){
		st_params = params;
		st_params->destroySelf = false;
	} else{
		st_params = (CdialogMessageParams *)memoryAllocEx( "MessageParams", MEMORY_USER, 0, sizeof( CdialogMessageParams ), PSP_SMEM_High, NULL );
		if( ! st_params ) return CG_ERROR_NOT_ENOUGH_MEMORY;
		st_params->destroySelf = true;
	}
	
	cdialogDevInitBaseParams( &(st_params->base) );
	
	/* 初期化 */
	st_params->data.title[0]   = '\0';
	st_params->data.options    = 0;
	st_params->data.message[0] = '\0';
	
	return CG_ERROR_OK;
}

CdialogMessageData *cdialogMessageGetData( void )
{
	return &(st_params->data);
}

CdialogStatus cdialogMessageGetStatus( void )
{
	if( ! st_params ) return CDIALOG_NONE;
	return st_params->base.status;
}

CdialogResult cdialogMessageGetResult( void )
{
	return st_params->base.result;
}

int cdialogMessageStart( unsigned short x, unsigned short y )
{
	int ret;
	
	if( ! cdialogDevLock() ) return CG_ERROR_ALREADY_RUNNING;
	ret = cdialogMessageStartNoLock( x, y );
	
	if( ret < 0 ) cdialogDevUnlock();
	
	return ret;
}

int cdialogMessageStartNoLock( unsigned short x, unsigned short y )
{
	int ret;
	unsigned short w, h;
	
	//st_params->base.status = CDIALOG_INIT;
	
	/* ダイアログの幅と高さを検出 */
	cdialog_message_count_width_and_height( st_params->data.message, &w, &h );
	
	/* 固定幅と固定行を加算 */
	st_params->base.width  = gbOffsetChar( w + 2 );
	st_params->base.height = gbOffsetLine( h + 6 );
	
	st_params->base.x = gbOffsetChar( x );
	st_params->base.y = gbOffsetLine( y );
	
	ret = cdialogDevPrepareToStart( &(st_params->base), st_params->data.options );
	if( ret != CG_ERROR_OK ) return ret;
	
	st_params->base.status = CDIALOG_VISIBLE;
	
	return 0;
}

int cdialogMessageUpdate( void )
{
	SceCtrlData pad;
	
	if( st_params->base.result != CDIALOG_UNKNOWN ){
		st_params->base.status = CDIALOG_SHUTDOWN;
		return 0;
	}
	
	cdialog_message_draw( &(st_params->base), &(st_params->data) );
	cdialogDevReadCtrlBuffer( &(st_params->base), &pad, NULL );
	
	if( pad.Buttons & PSP_CTRL_CIRCLE ){
		st_params->base.result = CDIALOG_ACCEPT;
	} else if( ( pad.Buttons & PSP_CTRL_CROSS ) && ( st_params->data.options & CDIALOG_MESSAGE_YESNO ) ){
		st_params->base.result = CDIALOG_CANCEL;
	}
	
	return 0;
}

int cdialogMessageShutdownStartNoLock( void )
{
	if( st_params->base.status != CDIALOG_SHUTDOWN ){
		st_params->base.result = CDIALOG_CANCEL;
		st_params->base.status = CDIALOG_SHUTDOWN;
	}
	
	cdialogDevPrepareToFinish( &(st_params->base) );
	
	return 0;
}

int cdialogMessageShutdownStart( void )
{
	int ret = cdialogMessageShutdownStartNoLock();
	cdialogDevUnlock();
	return ret;
}

void cdialogMessageDestroy( void )
{
	if( st_params->destroySelf ){
		memoryFree( st_params );
	}
	st_params = NULL;
}

static void cdialog_message_count_width_and_height( const char *str, unsigned short *width, unsigned short *height )
{
	unsigned short cur_width = 0;
	const char *start = str;
	const char *end   = NULL;
	
	*width  = 0;
	*height = 1;
	
	while( ( end = strchr( start, '\n' ) ) ){
		*height += 1;
		cur_width = end - start;
		
		if( cur_width > *width ) *width = cur_width;
		
		start = end + 1;
	}
	
	if( ! *width ){
		*width = strlen( str );
	} else{
		cur_width = strlen( start );
		if( cur_width > *width ) *width = cur_width;
	}
}

static void cdialog_message_draw( struct cdialog_dev_base_params *base, CdialogMessageData *data )
{
	const char *ans = data->options & CDIALOG_MESSAGE_YESNO ? CDIALOG_MESSAGE_ANS_YESNO : CDIALOG_MESSAGE_ANS_OK;
	
	/* 枠を描画 */
	gbFillRectRel( base->x, base->y, base->width, base->height, base->color->bg );
	gbLineRectRel( base->x, base->y, base->width, base->height, base->color->border );
	
	/* タイトル描画 */
	gbPrint(
		base->x + ( base->width >> 1 ) - gbOffsetChar( strlen( data->title ) >> 1 ),
		base->y + gbOffsetLine( 1 ),
		base->color->title,
		GB_TRANSPARENT,
		data->title
	);
	
	gbPrint(
		base->x + gbOffsetChar( 1 ),
		base->y + gbOffsetLine( 3 ),
		base->color->fg,
		GB_TRANSPARENT,
		data->message
	);
	
	gbPrint(
		base->x + ( base->width >> 1 ) - gbOffsetChar( strlen( ans ) >> 1 ),
		base->y + + base->height - gbOffsetLine( 2 ),
		base->color->fcfg,
		GB_TRANSPARENT,
		ans
	);
}
