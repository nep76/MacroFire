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
	{ "Deadzone radius: %d %s",     mfMenuDefGetNumberProc, &(st_deadzone.r),  { { .string = "Set deadzone radius"  }, { .string = "(0-128)"  }, { .integer = 3 }, { .integer = 1 }, { .integer = 128 } } },
	{ "Sensitivity    : %d %s",     mfMenuDefGetNumberProc, &(st_sensitivity), { { .string = "Set sensitivity rate" }, { .string = "% (0-200)"}, { .integer = 3 }, { .integer = 1 }, { .integer = 200 } } },
	{ NULL },
	{ "Origin X-coordinate: %d %s", mfMenuDefGetNumberProc, &(st_deadzone.x),  { { .string = "Set origin X-coordinate" }, { .string = "(0-255)" }, { .integer = 3 }, { .integer = 1 }, { .integer = 255 } } },
	{ "Origin Y-coordinate: %d %s", mfMenuDefGetNumberProc, &(st_deadzone.y),  { { .string = "Set origin Y-coordinate" }, { .string = "(0-255)" }, { .integer = 3 }, { .integer = 1 }, { .integer = 255 } } },
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
	if( CTRLPAD_IN_DEADZONE( abs( pad_data->Lx - st_deadzone.x ), abs( pad_data->Ly - st_deadzone.y ), st_deadzone.r ) ){
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
	
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine(  4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Analog stick sensitivity tuner." );
	pad_dupe = *pad_data;
	analogtuneTune( &pad_dupe, NULL );
	
	analog_dir = ctrlpadUtilGetAnalogDirection( pad_dupe.Lx, pad_dupe.Ly, 0 );
	
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 24 ),  MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Test of analog stick direction:" );
	gbPrint( gbOffsetChar( 32 + 5 ), gbOffsetLine( 23 ),  analog_dir & CTRLPAD_CTRL_ANALOG_UP    ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80" );
	gbPrint( gbOffsetChar( 32 + 7 ), gbOffsetLine( 24 ),  analog_dir & CTRLPAD_CTRL_ANALOG_RIGHT ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x81" );
	gbPrint( gbOffsetChar( 32 + 5 ), gbOffsetLine( 25 ),  analog_dir & CTRLPAD_CTRL_ANALOG_DOWN  ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x82" );
	gbPrint( gbOffsetChar( 32 + 3 ), gbOffsetLine( 24 ),  analog_dir & CTRLPAD_CTRL_ANALOG_LEFT  ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x83" );
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 27 ),  MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Test of analog stick movement:" );
	gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 28 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Untuned coordinate (X,Y) = ( %03d , %03d )", pad_data->Lx, pad_data->Ly );
	gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 29 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Tuned coordinate   (X,Y) = ( %03d , %03d )", pad_dupe.Lx, pad_dupe.Ly );
	
	switch( mfMenuUniDraw( gbOffsetChar( 5 ), gbOffsetLine( 6 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected, 0 ) ){
		case MR_CONTINUE:
		case MR_ENTER:
			break;
		case MR_BACK:
			//selected = 0;
			return MR_BACK;
	}
	
	return MR_CONTINUE;
}

int analogtuneGetOriginX( void )
{
	return st_deadzone.x;
}

int analogtuneGetOriginY( void )
{
	return st_deadzone.y;
}

int analogtuneGetDeadzone( void )
{
	return st_deadzone.r;
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
