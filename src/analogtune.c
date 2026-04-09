/*
	analogtune.c
*/

#include "analogtune.h"

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static int st_sensitivity = ANALOGTUNE_INIT_SENS;
static AnalogtuneDeadzone st_deadzone = {
	ANALOGTUNE_INIT_X,
	ANALOGTUNE_INIT_Y,
	ANALOGTUNE_INIT_R
};
	
static MfMenuItem st_menu_table[] = {
	{ MT_GET_NUMBERS, "Deadzone radius",     0, &(st_deadzone.r),  { { .string = "Set deadzone radius"  }, { .string = "(0-128)"  }, { .integer = 3 }, { .integer = 1 } } },
	{ MT_GET_NUMBERS, "Sensitivity    ",     0, &(st_sensitivity), { { .string = "Set sensitivity rate" }, { .string = "% (0-200)"}, { .integer = 3 }, { .integer = 1 } } },
	{ MT_NULL },
	{ MT_GET_NUMBERS, "Origin X-coordinate", 0, &(st_deadzone.x),  { { .string = "Set origin X-coordinate" }, { .string = "(0-255)" }, { .integer = 3 }, { .integer = 1 } } },
	{ MT_GET_NUMBERS, "Origin Y-coordinate", 0, &(st_deadzone.y),  { { .string = "Set origin Y-coordinate" }, { .string = "(0-255)" }, { .integer = 3 }, { .integer = 1 } } },
};

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void analogtune_normalize( void );

/*=============================================*/

void analogtuneLoadIni( IniUID ini )
{
	st_deadzone.x  = inimgrGetInt( ini, "Analogtune", "OriginX",     ANALOGTUNE_INIT_X );
	st_deadzone.y  = inimgrGetInt( ini, "Analogtune", "OriginY",     ANALOGTUNE_INIT_Y );
	st_deadzone.r  = inimgrGetInt( ini, "Analogtune", "Deadzone",    ANALOGTUNE_INIT_R );
	st_sensitivity = inimgrGetInt( ini, "Analogtune", "Sensitivity", ANALOGTUNE_INIT_SENS );
}

void analogtuneCreateIni( IniUID ini )
{
	inimgrSetInt( ini, "Analogtune", "OriginX",     ANALOGTUNE_INIT_X );
	inimgrSetInt( ini, "Analogtune", "OriginY",     ANALOGTUNE_INIT_Y );
	inimgrSetInt( ini, "Analogtune", "Deadzone",    ANALOGTUNE_INIT_R );
	inimgrSetInt( ini, "Analogtune", "Sensitivity", ANALOGTUNE_INIT_SENS );
}

void analogtuneTune( SceCtrlData *pad_data, void *argp )
{
	if(
		( ANALOGTUNE_SQUARE( abs( pad_data->Lx - st_deadzone.x ) ) + ANALOGTUNE_SQUARE( abs( pad_data->Ly - st_deadzone.y ) ) ) <=
		( ANALOGTUNE_SQUARE( st_deadzone.r ) * 2 )
	){
		pad_data->Lx = ANALOGTUNE_INIT_X;
		pad_data->Ly = ANALOGTUNE_INIT_Y;
	} else if( st_sensitivity != 100 ){
		int x = st_deadzone.x + ( ( pad_data->Lx - st_deadzone.x ) * st_sensitivity / 100 );
		int y = st_deadzone.y + ( ( pad_data->Ly - st_deadzone.y ) * st_sensitivity / 100 );
		
		if( x > ANALOGTUNE_MAX_COORD ){
			x = ANALOGTUNE_MAX_COORD;
		} else if( x < 0 ){
			x = 0;
		}
		
		if( y > ANALOGTUNE_MAX_COORD ){
			y = ANALOGTUNE_MAX_COORD;
		} else if( y < 0 ){
			y = 0;
		}
		
		pad_data->Lx = x;
		pad_data->Ly = y;
	}
	
	if(
		( st_deadzone.x < ANALOGTUNE_INIT_X && pad_data->Lx > st_deadzone.x && pad_data->Lx < ANALOGTUNE_INIT_X ) ||
		( st_deadzone.x > ANALOGTUNE_INIT_X && pad_data->Lx < st_deadzone.x && pad_data->Lx > ANALOGTUNE_INIT_X )
	){
		pad_data->Lx = ANALOGTUNE_INIT_X;
	}
	if(
		( st_deadzone.y < ANALOGTUNE_INIT_Y && pad_data->Ly > st_deadzone.y && pad_data->Ly < ANALOGTUNE_INIT_Y ) ||
		( st_deadzone.y > ANALOGTUNE_INIT_Y && pad_data->Ly < st_deadzone.y && pad_data->Ly > ANALOGTUNE_INIT_Y )
	){
		pad_data->Ly = ANALOGTUNE_INIT_Y;
	}
}

MfMenuRc analogtuneMenu( SceCtrlData *pad_data, void *argp )
{
	SceCtrlData pad_dupe;
	static int selected = 0;
	unsigned int analog_dir;
	
	analogtune_normalize();
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine(  4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Analog stick sensitivity tuner." );
	pad_dupe = *pad_data;
	analogtuneTune( &pad_dupe, NULL );
	
	analog_dir = ctrlpadUtilGetAnalogDirection( pad_dupe.Lx, pad_dupe.Ly );
	
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 24 ),  MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Test of analog stick direction:" );
	blitString( blitOffsetChar( 32 + 5 ), blitOffsetLine( 23 ),  analog_dir & CTRLPAD_CTRL_ANALOG_UP    ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80" );
	blitString( blitOffsetChar( 32 + 7 ), blitOffsetLine( 24 ),  analog_dir & CTRLPAD_CTRL_ANALOG_RIGHT ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x81" );
	blitString( blitOffsetChar( 32 + 5 ), blitOffsetLine( 25 ),  analog_dir & CTRLPAD_CTRL_ANALOG_DOWN  ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x82" );
	blitString( blitOffsetChar( 32 + 3 ), blitOffsetLine( 24 ),  analog_dir & CTRLPAD_CTRL_ANALOG_LEFT  ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x83" );
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 27 ),  MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Test of analog stick movement:" );
	blitStringf( blitOffsetChar( 5 ), blitOffsetLine( 28 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Untuned coordinate (X,Y) = ( %03d , %03d )", pad_data->Lx, pad_data->Ly );
	blitStringf( blitOffsetChar( 5 ), blitOffsetLine( 29 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Tuned coordinate   (X,Y) = ( %03d , %03d )", pad_dupe.Lx, pad_dupe.Ly );
	
	switch( mfMenuUniDraw( blitOffsetChar( 5 ), blitOffsetLine( 6 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected, 0 ) ){
		case MR_CONTINUE:
		case MR_ENTER:
			break;
		case MR_BACK:
			//selected = 0;
			return MR_BACK;
	}
	
	return MR_CONTINUE;
}

static void analogtune_normalize( void )
{
	
	if( st_deadzone.x > ANALOGTUNE_MAX_COORD ){
		st_deadzone.x = ANALOGTUNE_MAX_COORD;
	} else if( st_deadzone.x < 0 ){
		st_deadzone.x = 0;
	}
	
	if( st_deadzone.y > ANALOGTUNE_MAX_COORD ){
		st_deadzone.y = ANALOGTUNE_MAX_COORD;
	} else if( st_deadzone.y < 0 ){
		st_deadzone.y = 0;
	}
	
	if( st_deadzone.r > ANALOGTUNE_MAX_RADIUS ){
		st_deadzone.r = ANALOGTUNE_MAX_RADIUS;
	} else if( st_deadzone.r < 0 ){
		st_deadzone.r = 0;
	}
	
	if( st_sensitivity > ANALOGTUNE_MAX_SENS ){
		st_sensitivity = ANALOGTUNE_MAX_SENS;
	} else if( st_sensitivity < 0 ){
		st_sensitivity = 0;
	}
}
