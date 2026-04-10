/*
	analogtune.c
*/

#include "analogtune.h"

/*-----------------------------------------------
	ローカル変数と定数
-----------------------------------------------*/
static int st_sensitivity = ANALOGTUNE_INIT_SENS;
static bool st_analog_stick = ANALOGTUNE_INIT_MOVEMENT;
static AnalogtuneDeadzone st_deadzone = {
	PADUTIL_CENTER_X,
	PADUTIL_CENTER_Y,
	ANALOGTUNE_INIT_R
};
	
static MfMenuItem st_menu_table[] = {
	{ "Analog stick movement: %s",   mfMenuDefBoolProc,      &st_analog_stick,  { { .string = "OFF" }, { .string = "ON" } } },
	{ NULL },
	{ "Deadzone radius: %d %s",     mfMenuDefGetNumberProc, &(st_deadzone.r),  { { .string = "Set deadzone radius"  }, { .string = "(0-182)"  }, { .integer = 3 }, { .integer = 1 }, { .integer = PADUTIL_MAX_RADIUS } } },
	{ "Sensitivity    : %d %s",     mfMenuDefGetNumberProc, &(st_sensitivity), { { .string = "Set sensitivity rate" }, { .string = "% (0-200)"}, { .integer = 3 }, { .integer = 1 }, { .integer = ANALOGTUNE_MAX_SENS } } },
	{ NULL },
	{ "Origin X-coordinate: %d %s", mfMenuDefGetNumberProc, &(st_deadzone.x),  { { .string = "Set origin X-coordinate" }, { .string = "(0-255)" }, { .integer = 3 }, { .integer = 1 }, { .integer = PADUTIL_MAX_COORD } } },
	{ "Origin Y-coordinate: %d %s", mfMenuDefGetNumberProc, &(st_deadzone.y),  { { .string = "Set origin Y-coordinate" }, { .string = "(0-255)" }, { .integer = 3 }, { .integer = 1 }, { .integer = PADUTIL_MAX_COORD } } },
};

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void analogtune_normalize( void );

/*=============================================*/

void analogtuneLoadIni( IniUID ini )
{
	st_analog_stick = ANALOGTUNE_INIT_MOVEMENT;
	st_deadzone.x   = PADUTIL_CENTER_X;
	st_deadzone.y   = PADUTIL_CENTER_Y;
	st_deadzone.r   = ANALOGTUNE_INIT_R;
	st_sensitivity  = ANALOGTUNE_INIT_SENS;
	
	inimgrGetBool( ini, "Analogtune", "AnalogStick", &st_analog_stick );
	inimgrGetInt(  ini, "Analogtune", "OriginX",     (long *)&(st_deadzone.x) );
	inimgrGetInt(  ini, "Analogtune", "OriginY",     (long *)&(st_deadzone.y) );
	inimgrGetInt(  ini, "Analogtune", "Deadzone",    (long *)&(st_deadzone.r) );
	inimgrGetInt(  ini, "Analogtune", "Sensitivity", (long *)&(st_sensitivity) );
}

void analogtuneCreateIni( IniUID ini )
{
	inimgrSetBool( ini, "Analogtune", "AnalogStick", ANALOGTUNE_INIT_MOVEMENT );
	inimgrSetInt(  ini, "Analogtune", "OriginX",     PADUTIL_CENTER_X );
	inimgrSetInt(  ini, "Analogtune", "OriginY",     PADUTIL_CENTER_Y );
	inimgrSetInt(  ini, "Analogtune", "Deadzone",    ANALOGTUNE_INIT_R );
	inimgrSetInt(  ini, "Analogtune", "Sensitivity", ANALOGTUNE_INIT_SENS );
}

void analogtuneTune( SceCtrlData *pad_data, void *argp )
{
	if( ! st_analog_stick || PADUTIL_IN_DEADZONE( abs( pad_data->Lx - st_deadzone.x ), abs( pad_data->Ly - st_deadzone.y ), st_deadzone.r ) ){
		pad_data->Lx = PADUTIL_CENTER_X;
		pad_data->Ly = PADUTIL_CENTER_Y;
	} else if( st_sensitivity != 100 ){
		int x = st_deadzone.x + ( ( pad_data->Lx - st_deadzone.x ) * st_sensitivity / 100 );
		int y = st_deadzone.y + ( ( pad_data->Ly - st_deadzone.y ) * st_sensitivity / 100 );
		
		if( x > PADUTIL_MAX_COORD ){
			x = PADUTIL_MAX_COORD;
		} else if( x < 0 ){
			x = 0;
		}
		
		if( y > PADUTIL_MAX_COORD ){
			y = PADUTIL_MAX_COORD;
		} else if( y < 0 ){
			y = 0;
		}
		
		pad_data->Lx = x;
		pad_data->Ly = y;
	}
	
	if(
		( st_deadzone.x < PADUTIL_CENTER_X && pad_data->Lx > st_deadzone.x && pad_data->Lx < PADUTIL_CENTER_X ) ||
		( st_deadzone.x > PADUTIL_CENTER_X && pad_data->Lx < st_deadzone.x && pad_data->Lx > PADUTIL_CENTER_X )
	){
		pad_data->Lx = PADUTIL_CENTER_X;
	}
	if(
		( st_deadzone.y < PADUTIL_CENTER_Y && pad_data->Ly > st_deadzone.y && pad_data->Ly < PADUTIL_CENTER_Y ) ||
		( st_deadzone.y > PADUTIL_CENTER_Y && pad_data->Ly < st_deadzone.y && pad_data->Ly > PADUTIL_CENTER_Y )
	){
		pad_data->Ly = PADUTIL_CENTER_Y;
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
	
	analog_dir = padutilGetAnalogDirection( pad_dupe.Lx, pad_dupe.Ly, 0 );
	
	gbLineRectRel( gbOffsetChar( 60 ) - ( 128 >> 1 ) - 1, gbOffsetLine( 18 ) - ( 128 >> 1 ) - 1, ( 256 >> 1 ) + 1, ( 256 >> 1 ) + 1, MFM_TEXT_FGCOLOR );
	gbLineCircle( gbOffsetChar( 60 ) + ( ( st_deadzone.x - PADUTIL_CENTER_X ) >> 1 ), gbOffsetLine( 18 ) + ( ( st_deadzone.y - PADUTIL_CENTER_Y ) >> 1 ), st_deadzone.r >> 1, MFM_TEXT_FCCOLOR );
	gbPoint( gbOffsetChar( 60 ) + ( ( pad_data->Lx - PADUTIL_CENTER_X  ) >> 1 ), gbOffsetLine( 18 ) + ( ( pad_data->Ly - PADUTIL_CENTER_Y ) >> 1 ), MFM_TEXT_FCCOLOR );
	gbPoint( gbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ), gbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ), MFM_TEXT_FGCOLOR );
	
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 24 ),  MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Test of analog stick direction:" );
	gbPrint( gbOffsetChar( 32 + 5 ), gbOffsetLine( 23 ),  analog_dir & PADUTIL_CTRL_ANALOG_UP    ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80" );
	gbPrint( gbOffsetChar( 32 + 7 ), gbOffsetLine( 24 ),  analog_dir & PADUTIL_CTRL_ANALOG_RIGHT ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x81" );
	gbPrint( gbOffsetChar( 32 + 5 ), gbOffsetLine( 25 ),  analog_dir & PADUTIL_CTRL_ANALOG_DOWN  ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x82" );
	gbPrint( gbOffsetChar( 32 + 3 ), gbOffsetLine( 24 ),  analog_dir & PADUTIL_CTRL_ANALOG_LEFT  ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x83" );
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 27 ),  MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Test of analog stick movement:" );
	gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 28 ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, "Untuned coordinate (X,Y) = ( %03d , %03d )", pad_data->Lx, pad_data->Ly );
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
	
	if( st_deadzone.x > PADUTIL_MAX_COORD ){
		st_deadzone.x = PADUTIL_MAX_COORD;
	} else if( st_deadzone.x < 0 ){
		st_deadzone.x = 0;
	}
	
	if( st_deadzone.y > PADUTIL_MAX_COORD ){
		st_deadzone.y = PADUTIL_MAX_COORD;
	} else if( st_deadzone.y < 0 ){
		st_deadzone.y = 0;
	}
	
	if( st_deadzone.r > PADUTIL_MAX_RADIUS ){
		st_deadzone.r = PADUTIL_MAX_RADIUS;
	} else if( st_deadzone.r < 0 ){
		st_deadzone.r = 0;
	}
	
	if( st_sensitivity > ANALOGTUNE_MAX_SENS ){
		st_sensitivity = ANALOGTUNE_MAX_SENS;
	} else if( st_sensitivity < 0 ){
		st_sensitivity = 0;
	}
}
