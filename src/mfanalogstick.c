/*=========================================================

	mfanalogstick.c

	アナログスティックの調整。

=========================================================*/
#include "mfanalogstick.h"

/*=========================================================
	ローカル関数
=========================================================*/

/*=========================================================
	ローカル変数
=========================================================*/
static bool st_enable_movement;
static PadutilAnalogStick st_analogstick;

/*=========================================================
	関数
=========================================================*/
void *mfAnalogStickProc( MfMessage message )
{
	switch( message ){
		case MF_MS_INIT:
			st_enable_movement         = true;
			st_analogstick.deadzone    = 40;
			st_analogstick.sensitivity = 1.0f;
			st_analogstick.originX     = PADUTIL_CENTER_X;
			st_analogstick.originY     = PADUTIL_CENTER_Y;
			break;
		case MF_MS_INI_LOAD:
			return mfAnalogStickIniLoad;
		case MF_MS_INI_CREATE:
			return mfAnalogStickIniSave;
		case MF_MS_HOOK:
			return mfAnalogStickAdjust;
		case MF_MS_MENU:
			return mfAnalogStickMenu;
		default:
			break;
	}
	return NULL;
}

void mfAnalogStickIniLoad( IniUID ini, char *buf, size_t len )
{
	int deadzone    = st_analogstick.deadzone;
	int sensitivity = st_analogstick.sensitivity;
	int originx     = st_analogstick.originX;
	int originy     = st_analogstick.originY;
	
	st_enable_movement = true;
	
	inimgrGetBool( ini, "AnalogStick", "Movement",    &st_enable_movement );
	inimgrGetInt(  ini, "AnalogStick", "Deadzone",    &deadzone );
	inimgrGetInt(  ini, "AnalogStick", "Sensitivity", &sensitivity );
	inimgrGetInt(  ini, "AnalogStick", "OriginX",     &originx );
	inimgrGetInt(  ini, "AnalogStick", "OriginY",     &originy );
	
	st_analogstick.deadzone    = deadzone;
	st_analogstick.sensitivity = sensitivity / 100;
	st_analogstick.originX     = originx;
	st_analogstick.originY     = originy;
}

void mfAnalogStickIniSave( IniUID ini, char *buf, size_t len )
{
	inimgrSetBool( ini, "AnalogStick", "Movement",    true );
	inimgrSetInt(  ini, "AnalogStick", "Deadzone",    40 );
	inimgrSetInt(  ini, "AnalogStick", "Sensitivity", 100 );
	inimgrSetInt(  ini, "AnalogStick", "OriginX",     PADUTIL_CENTER_X );
	inimgrSetInt(  ini, "AnalogStick", "OriginY",     PADUTIL_CENTER_Y );
}

void mfAnalogStickAdjust( MfHookAction action, SceCtrlData *pad )
{
	if( st_enable_movement ){
		padutilAdjustAnalogStick( &st_analogstick, pad );
	} else{
		pad->Lx = PADUTIL_CENTER_X;
		pad->Ly = PADUTIL_CENTER_Y;
	}
}

void mfAnalogStickMenu( MfMenuMessage message )
{
	static MfMenuTable   *menu;
	static HeapUID       pheap;
	static unsigned char sensitivity;
	
	SceCtrlData *pad, pad_dupe;
	pad = mfMenuGetCurrentPadData();
	
	switch( message ){
		case MF_MM_INIT:
			dbgprint( "Creating analogstick menu" );
			menu = mfMenuCreateTables(
				1,
				7, 1
			);
			pheap = heapCreate( sizeof( MfCtrlDefGetNumberPref ) * 3 + HEAP_HEADER_SIZE * 3 );
			if( ! menu || ! pheap ){
				if( menu ) mfMenuDestroyTables( menu );
				if( pheap ) heapDestroy( pheap );
				return;
			}
			
			{
				MfCtrlDefGetNumberPref *pref_deadzone    = heapCalloc( pheap, sizeof( MfCtrlDefGetNumberPref ) );
				MfCtrlDefGetNumberPref *pref_sensitivity = heapCalloc( pheap, sizeof( MfCtrlDefGetNumberPref ) );
				MfCtrlDefGetNumberPref *pref_origin      = heapCalloc( pheap, sizeof( MfCtrlDefGetNumberPref ) );
				
				pref_deadzone->unit  = "(0-182)";
				pref_deadzone->max   = 182;
				
				pref_sensitivity->unit = "% (0-200)";
				pref_sensitivity->max  = 200;
				
				pref_origin->unit = "(0-255)";
				pref_origin->max  = 255;
				
				sensitivity = st_analogstick.sensitivity * 100;
				
				mfMenuSetTablePosition( menu, 1, gbOffsetChar( 5 ), gbOffsetLine( 6 ) );
				mfMenuSetTableEntry( menu, 1, 1, 1, "Analog stick movement", mfCtrlDefBool, &st_enable_movement, NULL );
				mfMenuSetTableEntry( menu, 1, 2, 1, "", NULL, NULL, NULL );
				mfMenuSetTableEntry( menu, 1, 3, 1, "Deadzone radius", mfCtrlDefGetNumber, &(st_analogstick.deadzone), pref_deadzone );
				mfMenuSetTableEntry( menu, 1, 4, 1, "Sensitivity    ", mfCtrlDefGetNumber, &sensitivity, pref_sensitivity );
				mfMenuSetTableEntry( menu, 1, 5, 1, "", NULL, NULL, NULL );
				mfMenuSetTableEntry( menu, 1, 6, 1, "Origin X-coordinate", mfCtrlDefGetNumber, &(st_analogstick.originX), pref_origin );
				mfMenuSetTableEntry( menu, 1, 7, 1, "Origin Y-coordinate", mfCtrlDefGetNumber, &(st_analogstick.originY), pref_origin );
			}
			mfMenuInitTable( menu );
			break;
		case MF_MM_TERM:
			mfMenuDestroyTables( menu );
			heapDestroy( pheap );
			return;
		default:
			st_analogstick.sensitivity = sensitivity / 100.0f;
			
			pad_dupe = *pad;
			mfAnalogStickAdjust( MF_INTERNAL, &pad_dupe );
			
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Analog stick sensitivity adjustment." );
			
			gbLineRectRel( gbOffsetChar( 60 ) - ( 128 >> 1 ) - 1, gbOffsetLine( 18 ) - ( 128 >> 1 ) - 1, ( 256 >> 1 ) + 2, ( 256 >> 1 ) + 2, MF_COLOR_TEXT_FG );
			gbLineCircle( gbOffsetChar( 60 ) + ( ( st_analogstick.originX - PADUTIL_CENTER_X ) >> 1 ), gbOffsetLine( 18 ) + ( ( st_analogstick.originY - PADUTIL_CENTER_Y ) >> 1 ), st_analogstick.deadzone >> 1, MF_COLOR_TEXT_FC );
			
			/* 調整前座標の位置を示す */
			gbLine(
				gbOffsetChar( 60 ) + ( ( pad->Lx - PADUTIL_CENTER_X  ) >> 1 ),
				gbOffsetLine( 18 ) - ( 127 >> 1 ) - 1,
				gbOffsetChar( 60 ) + ( ( pad->Lx - PADUTIL_CENTER_X  ) >> 1 ),
				gbOffsetLine( 18 ) + ( 127 >> 1 ) + 1,
				( MF_COLOR_TEXT_FC & 0x00ffffff ) | 0x66000000
			);
			gbLine(
				gbOffsetChar( 60 ) - ( 127 >> 1 ) - 1,
				gbOffsetLine( 18 ) + ( ( pad->Ly - PADUTIL_CENTER_Y ) >> 1 ),
				gbOffsetChar( 60 ) + ( 127 >> 1 ) + 1,
				gbOffsetLine( 18 ) + ( ( pad->Ly - PADUTIL_CENTER_Y ) >> 1 ),
				( MF_COLOR_TEXT_FC & 0x00ffffff ) | 0x66000000
			);
			gbPoint( gbOffsetChar( 60 ) + ( ( pad->Lx - PADUTIL_CENTER_X  ) >> 1 ), gbOffsetLine( 18 ) + ( ( pad->Ly - PADUTIL_CENTER_Y ) >> 1 ), MF_COLOR_TEXT_FC );
			
			/* 調整後座標の位置を示す */
			gbLine(
				gbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ),
				gbOffsetLine( 18 ) - ( 127 >> 1 ) - 1,
				gbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ),
				gbOffsetLine( 18 ) + ( 127 >> 1 ) + 1,
				( MF_COLOR_TEXT_FG & 0x00ffffff ) | 0x66000000
			);
			gbLine(
				gbOffsetChar( 60 ) - ( 127 >> 1 ) - 1,
				gbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ),
				gbOffsetChar( 60 ) + ( 127 >> 1 ) + 1,
				gbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ),
				( MF_COLOR_TEXT_FG & 0x00ffffff ) | 0x66000000
			);
			gbPoint( gbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ), gbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ), MF_COLOR_TEXT_FG );
			
			pad_dupe.Buttons |= padutilGetAnalogStickDirection( pad_dupe.Lx, pad_dupe.Ly, 0 );
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 24 ),  MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Test of analog stick direction:" );
			gbPrint( gbOffsetChar( 32 + 5 ), gbOffsetLine( 23 ),  pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_UP    ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "\x80" );
			gbPrint( gbOffsetChar( 32 + 7 ), gbOffsetLine( 24 ),  pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_RIGHT ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "\x81" );
			gbPrint( gbOffsetChar( 32 + 5 ), gbOffsetLine( 25 ),  pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_DOWN  ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "\x82" );
			gbPrint( gbOffsetChar( 32 + 3 ), gbOffsetLine( 24 ),  pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_LEFT  ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "\x83" );
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 27 ),  MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Test of analog stick movement:" );
			gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 28 ), MF_COLOR_TEXT_FC, MF_COLOR_TEXT_BG, "     Raw coordinate (X,Y) = ( %03d , %03d )", pad->Lx, pad->Ly );
			gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 29 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Adjusted coordinate (X,Y) = ( %03d , %03d )", pad_dupe.Lx, pad_dupe.Ly );
			
			if( ! mfMenuDrawTable( menu, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
			break;
	}
}
