/*=========================================================

	mfanalogstick.c

	アナログスティックの調整。
	
	他の連射やマクロ機能のような構造になっているが、
	ここの関数は、メニューから直接呼び出しているため変更する場合は注意する。

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
	int deadzone, sensitivity, originx, originy;
	st_enable_movement = true;
	
	inimgrGetBool( ini, "AnalogStick", "Movement", &st_enable_movement );
	if( inimgrGetInt(  ini, "AnalogStick", "Deadzone",    &deadzone )    ) st_analogstick.deadzone    = deadzone;
	if( inimgrGetInt(  ini, "AnalogStick", "Sensitivity", &sensitivity ) ) st_analogstick.sensitivity = (float)sensitivity / 100.0f;
	if( inimgrGetInt(  ini, "AnalogStick", "OriginX",     &originx )     ) st_analogstick.originX     = originx;
	if( inimgrGetInt(  ini, "AnalogStick", "OriginY",     &originy )     ) st_analogstick.originY     = originy;
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
			pheap = mfHeapCreate( 3, sizeof( MfCtrlDefGetNumberPref ) * 3 );
			if( ! menu || ! pheap ){
				if( menu ) mfMenuDestroyTables( menu );
				if( pheap ) mfHeapDestroy( pheap );
				return;
			}
			
			{
				MfCtrlDefGetNumberPref *pref_deadzone    = mfHeapCalloc( pheap, sizeof( MfCtrlDefGetNumberPref ) );
				MfCtrlDefGetNumberPref *pref_sensitivity = mfHeapCalloc( pheap, sizeof( MfCtrlDefGetNumberPref ) );
				MfCtrlDefGetNumberPref *pref_origin      = mfHeapCalloc( pheap, sizeof( MfCtrlDefGetNumberPref ) );
				
				pref_deadzone->unit  = "(0-182)";
				pref_deadzone->max   = 182;
				
				pref_sensitivity->unit = "% (0-200)";
				pref_sensitivity->max  = 200;
				
				pref_origin->unit = "(0-255)";
				pref_origin->max  = 255;
				
				sensitivity = st_analogstick.sensitivity * 100;
				
				mfMenuSetTablePosition( menu, 1, pbOffsetChar( 5 ), pbOffsetLine( 6 ) );
				mfMenuSetTableEntry( menu, 1, 1, 1, MF_STR_ASA_MOVEMENT, mfCtrlDefBool, &st_enable_movement, NULL );
				mfMenuSetTableEntry( menu, 1, 2, 1, "", NULL, NULL, NULL );
				mfMenuSetTableEntry( menu, 1, 3, 1, MF_STR_ASA_DEADZONE, mfCtrlDefGetNumber, &(st_analogstick.deadzone), pref_deadzone );
				mfMenuSetTableEntry( menu, 1, 4, 1, MF_STR_ASA_SENSITIVITY, mfCtrlDefGetNumber, &sensitivity, pref_sensitivity );
				mfMenuSetTableEntry( menu, 1, 5, 1, "", NULL, NULL, NULL );
				mfMenuSetTableEntry( menu, 1, 6, 1, MF_STR_ASA_ORIGIN_X, mfCtrlDefGetNumber, &(st_analogstick.originX), pref_origin );
				mfMenuSetTableEntry( menu, 1, 7, 1, MF_STR_ASA_ORIGIN_Y, mfCtrlDefGetNumber, &(st_analogstick.originY), pref_origin );
			}
			mfMenuInitTable( menu );
			break;
		case MF_MM_TERM:
			mfMenuDestroyTables( menu );
			mfHeapDestroy( pheap );
			return;
		default:
			st_analogstick.sensitivity = sensitivity / 100.0f;
			
			pad_dupe = *pad;
			mfAnalogStickAdjust( MF_INTERNAL, &pad_dupe );
			
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_ASA_DESC );
			
			pbLineRectRel( pbOffsetChar( 60 ) - ( 128 >> 1 ) - 1, pbOffsetLine( 18 ) - ( 128 >> 1 ) - 1, ( 256 >> 1 ) + 2, ( 256 >> 1 ) + 2, MF_COLOR_TEXT_FG );
			pbLineCircle( pbOffsetChar( 60 ) + ( ( st_analogstick.originX - PADUTIL_CENTER_X ) >> 1 ), pbOffsetLine( 18 ) + ( ( st_analogstick.originY - PADUTIL_CENTER_Y ) >> 1 ), st_analogstick.deadzone >> 1, MF_COLOR_TEXT_FC );
			
			/* 調整前座標の位置を示す */
			pbLine(
				pbOffsetChar( 60 ) + ( ( pad->Lx - PADUTIL_CENTER_X  ) >> 1 ),
				pbOffsetLine( 18 ) - ( 127 >> 1 ) - 1,
				pbOffsetChar( 60 ) + ( ( pad->Lx - PADUTIL_CENTER_X  ) >> 1 ),
				pbOffsetLine( 18 ) + ( 127 >> 1 ) + 1,
				( MF_COLOR_TEXT_FC & 0x00ffffff ) | 0x66000000
			);
			pbLine(
				pbOffsetChar( 60 ) - ( 127 >> 1 ) - 1,
				pbOffsetLine( 18 ) + ( ( pad->Ly - PADUTIL_CENTER_Y ) >> 1 ),
				pbOffsetChar( 60 ) + ( 127 >> 1 ) + 1,
				pbOffsetLine( 18 ) + ( ( pad->Ly - PADUTIL_CENTER_Y ) >> 1 ),
				( MF_COLOR_TEXT_FC & 0x00ffffff ) | 0x66000000
			);
			pbPoint( pbOffsetChar( 60 ) + ( ( pad->Lx - PADUTIL_CENTER_X  ) >> 1 ), pbOffsetLine( 18 ) + ( ( pad->Ly - PADUTIL_CENTER_Y ) >> 1 ), MF_COLOR_TEXT_FC );
			
			/* 調整後座標の位置を示す */
			pbLine(
				pbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ),
				pbOffsetLine( 18 ) - ( 127 >> 1 ) - 1,
				pbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ),
				pbOffsetLine( 18 ) + ( 127 >> 1 ) + 1,
				( MF_COLOR_TEXT_FG & 0x00ffffff ) | 0x66000000
			);
			pbLine(
				pbOffsetChar( 60 ) - ( 127 >> 1 ) - 1,
				pbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ),
				pbOffsetChar( 60 ) + ( 127 >> 1 ) + 1,
				pbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ),
				( MF_COLOR_TEXT_FG & 0x00ffffff ) | 0x66000000
			);
			pbPoint( pbOffsetChar( 60 ) + ( ( pad_dupe.Lx - PADUTIL_CENTER_X  ) >> 1 ), pbOffsetLine( 18 ) + ( ( pad_dupe.Ly - PADUTIL_CENTER_Y ) >> 1 ), MF_COLOR_TEXT_FG );
			
			pad_dupe.Buttons |= padutilGetAnalogStickDirection( pad_dupe.Lx, pad_dupe.Ly, 0 );
			{
				unsigned short test_cross_offset = pbMeasureString( MF_STR_ASA_TEST_DIR ) + pbOffsetChar( 1 );
				pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 24 ),  MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_ASA_TEST_DIR );
				pbPrint( test_cross_offset + pbOffsetChar( 5 ), pbOffsetLine( 23 ), pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_UP    ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, PB_SYM_PSP_UP );
				pbPrint( test_cross_offset + pbOffsetChar( 7 ), pbOffsetLine( 24 ), pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_RIGHT ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, PB_SYM_PSP_RIGHT );
				pbPrint( test_cross_offset + pbOffsetChar( 5 ), pbOffsetLine( 25 ), pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_DOWN  ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, PB_SYM_PSP_DOWN );
				pbPrint( test_cross_offset + pbOffsetChar( 3 ), pbOffsetLine( 24 ), pad_dupe.Buttons & PADUTIL_CTRL_ANALOG_LEFT  ? MF_COLOR_TEXT_FC : MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, PB_SYM_PSP_LEFT );
			}
			pbPrint( pbOffsetChar( 3 ), pbOffsetLine( 27 ),  MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, MF_STR_ASA_TEST_MOVE );
			pbPrintf( pbOffsetChar( 5 ), pbOffsetLine( 28 ), MF_COLOR_TEXT_FC, MF_COLOR_TEXT_BG, "%s (X,Y) = ( %03d , %03d )", MF_STR_ASA_TEST_MOVE_RAW, pad->Lx, pad->Ly );
			pbPrintf( pbOffsetChar( 5 ), pbOffsetLine( 29 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "%s (X,Y) = ( %03d , %03d )", MF_STR_ASA_TEST_MOVE_ADJ, pad_dupe.Lx, pad_dupe.Ly );
			
			if( ! mfMenuDrawTable( menu, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
			break;
	}
}

const PadutilAnalogStick *mfAnalogStickGetContext( void )
{
	return &st_analogstick;
}
