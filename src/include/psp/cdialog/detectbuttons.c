/*=========================================================

	detectbuttons.c

	ボタンの組み合わせ取得。

=========================================================*/
#include "detectbuttons.h"

/*=========================================================
	ローカルマクロ
=========================================================*/
#define CDIALOG_DETECTBUTTONS_STR_DETECT "  \x85  : Starting to detect"
#define CDIALOG_DETECTBUTTONS_STR_CLEAR  "  \x87  : Clear current"
#define CDIALOG_DETECTBUTTONS_STR_CANCEL "  \x86  : Cancel"
#define CDIALOG_DETECTBUTTONS_STR_START  "START: Accept"

/*=========================================================
	ローカル宣言
=========================================================*/

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
		st_params = (CdialogDetectbuttonsParams *)memoryAlloc( sizeof( CdialogDetectbuttonsParams ) );
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
	
	st_params->base.x      = gbOffsetChar( x );
	st_params->base.y      = gbOffsetLine( y );
	st_params->base.width  = strlen( st_params->data.title );
	if( st_params->base.width < 35 ) st_params->base.width = 35;
	st_params->base.width  = gbOffsetChar( st_params->base.width + 2 );
	st_params->base.height = gbOffsetLine( 14 );
	
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
		PadutilButtons target_buttons = realbuttons & ( padutilSetPad( PADUTIL_PAD_NORMAL_BUTTONS | PADUTIL_PAD_ANALOGDIR_BUTTONS ) | padutilSetHprm( PADUTIL_HPRM_NORMAL_KEYS ) );
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
		if( pad.Buttons & PSP_CTRL_CIRCLE ){
			st_params->work.buttons        = 0;
			st_params->work.detecting      = true;
			st_params->work.waitForRelease = true;
			st_params->work.buf[0]         = '\0';
		} else if( pad.Buttons & PSP_CTRL_CROSS ){
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
	
	padutilDestroyButtonList( st_params->work.buttonNames );
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
	
	gbLineRel(
		base->x + gbOffsetChar( 1 ),
		base->y + gbOffsetLine( 10 ),
		base->width - ( gbOffsetChar( 2 ) ),
		0,
		base->color->fg
	);
	
	gbPrint(
		base->x + gbOffsetChar( 1 ),
		base->y + gbOffsetLine( 11 ),
		base->color->fg,
		GB_TRANSPARENT,
		"Current buttons:"
	);
	
	gbPrint(
		base->x + gbOffsetChar( 3 ),
		base->y + gbOffsetLine( 12 ),
		base->color->fg,
		base->color->fcbg,
		work->buf
	);
	
	if( work->detecting ){
		gbPrint(
			base->x + gbOffsetChar( 1 ),
			base->y + gbOffsetLine( 3 ),
			base->color->fg,
			GB_TRANSPARENT,
			"Please press the button combination.\n\nWhen you want to exit the dialog,\npress the already pressed button."
		);
		
		gbPrint(
			base->x + gbOffsetChar( 18 ),
			base->y + gbOffsetLine( 11 ),
			base->color->fcfg,
			GB_TRANSPARENT,
			"Now detecting..."
		);
	} else{
		gbPrint(
			base->x + gbOffsetChar( 1 ),
			base->y + gbOffsetLine( 3 ),
			base->color->fg,
			GB_TRANSPARENT,
			"Please choose one."
		);
		
		gbPrintf(
			base->x + gbOffsetChar( 1 ),
			base->y + gbOffsetLine( 5 ),
			base->color->fg,
			GB_TRANSPARENT,
			"%s\n%s\n%s\n%s",
			CDIALOG_DETECTBUTTONS_STR_DETECT,
			CDIALOG_DETECTBUTTONS_STR_CLEAR,
			CDIALOG_DETECTBUTTONS_STR_CANCEL,
			CDIALOG_DETECTBUTTONS_STR_START
		);
	}
}
