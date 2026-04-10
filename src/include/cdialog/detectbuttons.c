/*=========================================================

	detectbuttons.c

	ボタンの組み合わせ取得。

=========================================================*/
#include "detectbuttons.h"

/*=========================================================
	ローカル関数
=========================================================*/
void cdialog_detectbuttons_draw( struct cdialog_dev_base_params *base, CdialogDetectbuttonsData *data, struct cdialog_detectbuttons_work *work );

/*=========================================================
	ローカル変数
=========================================================*/
static CdialogDetectbuttonsParams *st_params;

/*=========================================================
	関数
=========================================================*/
int cdialogDetectbuttonsInit( CdialogDetectbuttonsParams *params )
{
	if( st_params ) return CG_ERROR_ALREADY_INITIALIZED;
	
	if( params ){
		st_params = params;
		st_params->destroySelf = false;
	} else{
		st_params = (CdialogDetectbuttonsParams *)memoryExalloc( "DetectbuttonsParams", MEMORY_USER, 0, sizeof( CdialogDetectbuttonsParams ), PSP_SMEM_High, NULL );
		if( ! st_params ) return CG_ERROR_NOT_ENOUGH_MEMORY;
		st_params->destroySelf = true;
	}
	
	cdialogDevInitBaseParams( &(st_params->base) );
	
	/* 初期化 */
	st_params->data.title[0]     = '\0';
	st_params->data.options      = 0;
	st_params->data.availButtons = 0;
	st_params->data.buttons      = NULL;
	
	return CG_ERROR_OK;
}

CdialogDetectbuttonsData *cdialogDetectbuttonsGetData( void )
{
	return &(st_params->data);
}

CdialogStatus cdialogDetectbuttonsGetStatus( void )
{
	return st_params->base.status;
}

CdialogResult cdialogDetectbuttonsGetResult( void )
{
	return st_params->base.result;
}

int cdialogDetectbuttonsStart( unsigned short x, unsigned short y )
{
	int ret;
	
	if( ! cdialogDevLock() ) return CG_ERROR_ALREADY_RUNNING;
	ret = cdialogDetectbuttonsStartNoLock( x, y );
	
	if( ret < 0 ) cdialogDevUnlock();
	
	return ret;
}

int cdialogDetectbuttonsStartNoLock( unsigned short x, unsigned short y )
{
	int ret;
	
	st_params->base.status = CDIALOG_INIT;
	
	st_params->showMessage = false;
	
	st_params->work.detecting      = false;
	st_params->work.waitForRelease = false;
	
	st_params->work.buttons        = *(st_params->data.buttons);
	st_params->work.buf[0]         = '\0';
	st_params->work.buttonNames    = padutilCreateButtonSymbols();
	if( ! st_params->work.buttonNames ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	padutilGetButtonNamesByCode( st_params->work.buttonNames, st_params->work.buttons, " ", 0, st_params->work.buf, CDIALOG_DETECTBUTTONS_BUF_LENGTH );
	
	st_params->base.x      = pbOffsetChar( x );
	st_params->base.y      = pbOffsetLine( y );
	st_params->base.width  = strlen( st_params->data.title );
	if( st_params->base.width < 35 ) st_params->base.width = 35;
	st_params->base.width  = pbOffsetChar( st_params->base.width + 2 );
	st_params->base.height = pbOffsetLine( 14 );
	
	ret = cdialogDevPrepareToStart( &(st_params->base), st_params->data.options );
	if( ret != CG_ERROR_OK ) return ret;
	
	st_params->base.status = CDIALOG_VISIBLE;
	
	return 0;
}

int cdialogDetectbuttonsUpdate( void )
{
	SceCtrlData pad;
	PadutilButtons realbuttons;
	
	if( st_params->base.result != CDIALOG_UNKNOWN ){
		st_params->base.status = CDIALOG_SHUTDOWN;
		return 0;
	}
	
	cdialog_detectbuttons_draw( &(st_params->base), &(st_params->data), &(st_params->work) );
	realbuttons = cdialogDevReadCtrlBuffer( &(st_params->base), &pad, NULL ) & st_params->data.availButtons;
	
	if( st_params->work.detecting ){
		PadutilButtons target_buttons = realbuttons;// & ( padutilSetPad( PADUTIL_PAD_NORMAL_BUTTONS | PADUTIL_PAD_ANALOGDIR_BUTTONS ) | padutilSetHprm( PADUTIL_HPRM_NORMAL_KEYS ) );
		bool detect = false;
		
		if( ! target_buttons ){
			if( st_params->work.waitForRelease ){
				st_params->work.waitForRelease = false;
			}
		} else if( ! st_params->work.waitForRelease ){
			 if( target_buttons & st_params->work.buttons ){
				st_params->work.detecting = false;
			} else{
				detect = true;
			}
		} else{
			detect = true;
		}
		
		/* 最初のボタン入力を無視する */
		if( detect && ( st_params->work.buttons || ! st_params->work.waitForRelease ) && ( ( st_params->work.buttons | target_buttons ) > st_params->work.buttons ) ){
			st_params->work.waitForRelease = true;
			st_params->work.buttons |= target_buttons;
			padutilGetButtonNamesByCode( st_params->work.buttonNames, st_params->work.buttons, " ", 0, st_params->work.buf, CDIALOG_DETECTBUTTONS_BUF_LENGTH );
		}
	} else{
		if( pad.Buttons & cdialogDevAcceptButton() ){
			st_params->work.buttons        = 0;
			st_params->work.detecting      = true;
			st_params->work.waitForRelease = true;
			st_params->work.buf[0]         = '\0';
		} else if( pad.Buttons & cdialogDevCancelButton() ){
			st_params->base.result = CDIALOG_CANCEL;
		} else if( pad.Buttons & PSP_CTRL_SQUARE ){
			st_params->work.buttons = 0;
			st_params->work.buf[0]  = '\0';
		} else if( pad.Buttons & PSP_CTRL_START ){
			*(st_params->data.buttons) = st_params->work.buttons;
			st_params->base.result = CDIALOG_ACCEPT;
		}
	}
	
	return 0;
}

int cdialogDetectbuttonsShutdownStartNoLock( void )
{
	if( st_params->base.status != CDIALOG_SHUTDOWN ){
		st_params->base.result = CDIALOG_CANCEL;
		st_params->base.status = CDIALOG_SHUTDOWN;
	}
	
	padutilDestroyButtonSymbols();
	cdialogDevPrepareToFinish( &(st_params->base) );
	
	return 0;
}

int cdialogDetectbuttonsShutdownStart( void )
{
	int ret = cdialogDetectbuttonsShutdownStartNoLock();
	cdialogDevUnlock();
	return ret;
}

void cdialogDetectbuttonsDestroy( void )
{
	if( st_params->destroySelf ){
		memoryFree( st_params );
	}
	st_params = NULL;
}

void cdialog_detectbuttons_draw( struct cdialog_dev_base_params *base, CdialogDetectbuttonsData *data, struct cdialog_detectbuttons_work *work )
{
	/* 枠を描画 */
	pbFillRectRel( base->x, base->y, base->width, base->height, base->color->bg );
	pbLineRectRel( base->x, base->y, base->width, base->height, base->color->border );
	
	/* タイトル描画 */
	pbPrint(
		base->x + ( base->width >> 1 ) - ( pbMeasureString( data->title ) >> 1 ),
		base->y + pbOffsetLine( 1 ),
		base->color->title,
		PB_TRANSPARENT,
		data->title
	);
	
	pbLineRel(
		base->x + pbOffsetChar( 1 ),
		base->y + pbOffsetLine( 10 ),
		base->width - ( pbOffsetChar( 2 ) ),
		0,
		base->color->fg
	);
	
	pbPrint(
		base->x + pbOffsetChar( 1 ),
		base->y + pbOffsetLine( 11 ),
		base->color->fg,
		PB_TRANSPARENT,
		CDIALOG_STR_DETECTBUTTONS_CURRENT_BUTTONS
	);
	
	pbPrint(
		base->x + pbOffsetChar( 3 ),
		base->y + pbOffsetLine( 12 ),
		base->color->fg,
		base->color->fcbg,
		work->buf
	);
	
	if( work->detecting ){
		pbPrint(
			base->x + pbOffsetChar( 1 ),
			base->y + pbOffsetLine( 3 ),
			base->color->fg,
			PB_TRANSPARENT,
			CDIALOG_STR_DETECTBUTTONS_DETECT_DESC
		);
		
		pbPrint(
			base->x + pbOffsetChar( 18 ),
			base->y + pbOffsetLine( 11 ),
			base->color->fcfg,
			PB_TRANSPARENT,
			CDIALOG_STR_DETECTBUTTONS_NOW_DETECTING
		);
	} else{
		pbPrint(
			base->x + pbOffsetChar( 1 ),
			base->y + pbOffsetLine( 3 ),
			base->color->fg,
			PB_TRANSPARENT,
			CDIALOG_STR_DETECTBUTTONS_DESC
		);
		
		pbPrintf(
			base->x + pbOffsetChar( 1 ),
			base->y + pbOffsetLine( 5 ),
			base->color->fg,
			PB_TRANSPARENT,
			"  %s  : %s\n"
			"  %s  : %s\n"
			"  %s  : %s\n"
			"START: %s",
			cdialogDevAcceptSymbol(), CDIALOG_STR_DETECTBUTTONS_START,
			PB_SYM_PSP_SQUARE,        CDIALOG_STR_DETECTBUTTONS_CLEAR,
			cdialogDevCancelSymbol(), CDIALOG_STR_DETECTBUTTONS_CANCEL,
			CDIALOG_STR_DETECTBUTTONS_ACCEPT
		);
	}
}
