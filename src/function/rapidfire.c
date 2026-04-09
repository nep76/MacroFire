/*
	RapidFire
*/

#include "rapidfire.h"

static RapidfireConf st_rfconf[] = {
	{ PSP_CTRL_CIRCLE,   0 },
	{ PSP_CTRL_CROSS,    0 },
	{ PSP_CTRL_SQUARE,   0 },
	{ PSP_CTRL_TRIANGLE, 0 },
	{ PSP_CTRL_UP,       0 },
	{ PSP_CTRL_RIGHT,    0 },
	{ PSP_CTRL_DOWN,     0 },
	{ PSP_CTRL_LEFT,     0 },
	{ PSP_CTRL_LTRIGGER, 0 },
	{ PSP_CTRL_RTRIGGER, 0 },
	{ PSP_CTRL_START,    0 },
	{ PSP_CTRL_SELECT,   0 }
};

static short st_rapid = 0;

static int st_release_delay = 2;
static int st_press_delay   = 2;

static int st_wait_cnt = 0;

#define RAPIDFIRE_MODE_NORMAL     0
#define RAPIDFIRE_MODE_SEMI_RAPID 1
#define RAPIDFIRE_MODE_AUTO_RAPID 2
#define RAPIDFIRE_MODE_AUTO_HOLD  3

#define RAPIDFIRE_LOAD 19
#define RAPIDFIRE_SAVE 20
static MfMenuItem st_rfMenuTable[] = {
	{ MT_OPTION, &(st_rfconf[0].mode),  "Circle  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[1].mode),  "Cross   ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[2].mode),  "Square  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[3].mode),  "Triangle", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_OPTION, &(st_rfconf[4].mode),  "Up      ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[5].mode),  "Right   ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[6].mode),  "Down    ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[7].mode),  "Left    ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_OPTION, &(st_rfconf[8].mode),  "L-trig  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[9].mode),  "R-trig  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_OPTION, &(st_rfconf[10].mode), "START   ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	{ MT_OPTION, &(st_rfconf[11].mode), "SELECT  ", { "NORMAL", "RAPID", "AUTO-RAPID", "AUTO-HOLD", 0 } },
	
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_GET_DIGITS, &(st_release_delay), "Release delay", { "Set release delay", "delay", "2" } },
	{ MT_GET_DIGITS, &(st_press_delay),   "Press delay  ", { "Set press delay",  "delay", "2" } },
	
	{ MT_BORDER, 0, 0, { 0 } },
	{ MT_ANCHOR, 0, "Load from MemoryStick", { 0 } },
	{ MT_ANCHOR, 0, "Save to MemoryStick",   { 0 } }

};

static void rapidfire_load( void )
{
	CmndlgOpenFilename cofn;
	CmndlgGetFilenameRc cgfrc;
	FilehUID fuid;
	
	char dir[256]  = { 0 };
	char name[128] = { 0 };
	char *path;
	
	char record[RAPIDFIRE_DATA_RECORD_MAXLEN];
	char *did, *value, *saveptr;
	
	cofn.dirPath     = dir;
	cofn.dirPathMax  = sizeof( dir );
	cofn.fileName    = name;
	cofn.fileNameMax = sizeof( name );
	
	cgfrc = cmndlgGetSaveFilename( "Load a rapidfire settings", "ms0:", &cofn );
	
	mfClearColor( MENU_BGCOLOR );
	mfDrawMainFrame();
	
	if( cgfrc == CMNDLG_GETFILENAME_CANCEL ) return;
	
	path = (char *)memsceMalloc( strlen( dir ) + strlen( name ) + 1 );
	if( ! path ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to create loading path." );
		mfWaitScreenReload( RAPIDFIRE_ERROR_DISPLAY_SEC );
		return;
	}
	sprintf( path, "%s/%s", dir, name );
	
	fuid = filehOpen( path, PSP_O_RDONLY, 0777 );
	if( ! fuid || filehGetLastError( fuid ) < 0){
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to load %s: %x-%x", path, filehGetLastError( fuid ), filehGetLastSystemError( fuid ) );
		mfWaitScreenReload( RAPIDFIRE_ERROR_DISPLAY_SEC );
		goto DESTROY;
	}
	
	/* シグネチャを読み込み、バーションをチェック */
	filehReadln( fuid, record, sizeof( record ) );
	uclcToUpper( record );
	did   = strtok_r( record,  RAPIDFIRE_DATA_RECORD_SEPARATOR, &saveptr );
	value = strtok_r( NULL, RAPIDFIRE_DATA_RECORD_SEPARATOR, &saveptr );
	if( ! did || ! value || strcmp( did, RAPIDFIRE_DATA_SIGNATURE ) != 0 ){
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "The file's signature is invalid.", did, RAPIDFIRE_DATA_SIGNATURE );
		mfWaitScreenReload( RAPIDFIRE_ERROR_DISPLAY_SEC );
		goto DESTROY;
	} else{
		int ver = strtol( value, NULL, 10 );
		if( ver > RAPIDFIRE_DATA_VERSION ){
			blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "The file's version is not supported." );
			mfWaitScreenReload( RAPIDFIRE_ERROR_DISPLAY_SEC );
			goto DESTROY;
		}
	}
	
	blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Loading from %s...", path );
	while( filehReadln( fuid, record, sizeof( record ) ) ){
		uclcToUpper( record );
		did   = strtok_r( record,  RAPIDFIRE_DATA_RECORD_SEPARATOR, &saveptr );
		value = strtok_r( NULL, RAPIDFIRE_DATA_RECORD_SEPARATOR, &saveptr );
		
		if( ! did || ! value ) continue;
		
		if( strcmp( did, RAPIDFIRE_DID_CIRCLE ) == 0 ){
			st_rfconf[0].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_CROSS ) == 0 ){
			st_rfconf[1].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_SQUARE ) == 0 ){
			st_rfconf[2].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_TRIANGLE ) == 0 ){
			st_rfconf[3].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_UP ) == 0 ){
			st_rfconf[4].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_RIGHT ) == 0 ){
			st_rfconf[5].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_DOWN ) == 0 ){
			st_rfconf[6].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_LEFT ) == 0 ){
			st_rfconf[7].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_LTRIGGER ) == 0 ){
			st_rfconf[8].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_RTRIGGER ) == 0 ){
			st_rfconf[9].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_START ) == 0 ){
			st_rfconf[10].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_SELECT ) == 0 ){
			st_rfconf[11].mode = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_RDELAY ) == 0 ){
			st_release_delay = strtol( value, NULL, 10 );
		} else if( strcmp( did, RAPIDFIRE_DID_PDELAY ) == 0 ){
			st_press_delay = strtol( value, NULL, 10 );
		} else{
			continue;
		}
	}
	
	filehClose( fuid );
	
	goto DESTROY;
	
	DESTROY:
		memsceFree( path );
		return;
}

static void rapidfire_save( void )
{
	CmndlgOpenFilename cofn;
	CmndlgGetFilenameRc cgfrc;
	FilehUID fuid;
	
	int i;
	char dir[256]  = { 0 };
	char name[128] = { 0 };
	char *path;
	
	cofn.dirPath     = dir;
	cofn.dirPathMax  = sizeof( dir );
	cofn.fileName    = name;
	cofn.fileNameMax = sizeof( name );
	
	cgfrc = cmndlgGetSaveFilename( "Save the rapidfire settings as ...", "ms0:", &cofn );
	
	mfClearColor( MENU_BGCOLOR );
	mfDrawMainFrame();
	
	if( cgfrc == CMNDLG_GETFILENAME_CANCEL ) return;
	
	path = (char *)memsceMalloc( strlen( dir ) + strlen( name ) + 1 );
	if( ! path ){
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to create saving path." );
		mfWaitScreenReload( RAPIDFIRE_ERROR_DISPLAY_SEC );
		return;
	}
	sprintf( path, "%s/%s", dir, name );
	
	fuid = filehOpen( path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if( ! fuid || filehGetLastError( fuid ) < 0){
		blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FCCOLOR, MENU_BGCOLOR, "Failed to save %s: %x-%x", path, filehGetLastError( fuid ), filehGetLastSystemError( fuid ) );
		mfWaitScreenReload( RAPIDFIRE_ERROR_DISPLAY_SEC );
		goto DESTROY;
	}
	
	blitStringf( blitOffsetChar( 3 ), blitOffsetLine( 2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Saving to %s...", path );
	
	/* 先頭にシグネチャを書き込む */
	filehWritef( fuid, RAPIDFIRE_DATA_RECORD_MAXLEN, "%s%s%d\n\n", RAPIDFIRE_DATA_SIGNATURE, RAPIDFIRE_DATA_RECORD_SEPARATOR, RAPIDFIRE_DATA_VERSION );
	
	for( i = 0; i < ARRAY_NUM( st_rfconf ); i++ ){
		switch( st_rfconf[i].button ){
			case PSP_CTRL_CIRCLE:
				filehWrite( fuid, RAPIDFIRE_DID_CIRCLE, strlen( RAPIDFIRE_DID_CIRCLE ) );
				break;
			case PSP_CTRL_CROSS:
				filehWrite( fuid, RAPIDFIRE_DID_CROSS, strlen( RAPIDFIRE_DID_CROSS ) );
				break;
			case PSP_CTRL_SQUARE:
				filehWrite( fuid, RAPIDFIRE_DID_SQUARE, strlen( RAPIDFIRE_DID_SQUARE ) );
				break;
			case PSP_CTRL_TRIANGLE:
				filehWrite( fuid, RAPIDFIRE_DID_TRIANGLE, strlen( RAPIDFIRE_DID_TRIANGLE ) );
				break;
			case PSP_CTRL_UP:
				filehWrite( fuid, RAPIDFIRE_DID_UP, strlen( RAPIDFIRE_DID_UP ) );
				break;
			case PSP_CTRL_RIGHT:
				filehWrite( fuid, RAPIDFIRE_DID_RIGHT, strlen( RAPIDFIRE_DID_RIGHT ) );
				break;
			case PSP_CTRL_DOWN:
				filehWrite( fuid, RAPIDFIRE_DID_DOWN, strlen( RAPIDFIRE_DID_DOWN ) );
				break;
			case PSP_CTRL_LEFT:
				filehWrite( fuid, RAPIDFIRE_DID_LEFT, strlen( RAPIDFIRE_DID_LEFT ) );
				break;
			case PSP_CTRL_LTRIGGER:
				filehWrite( fuid, RAPIDFIRE_DID_LTRIGGER, strlen( RAPIDFIRE_DID_LTRIGGER ) );
				break;
			case PSP_CTRL_RTRIGGER:
				filehWrite( fuid, RAPIDFIRE_DID_RTRIGGER, strlen( RAPIDFIRE_DID_RTRIGGER ) );
				break;
			case PSP_CTRL_START:
				filehWrite( fuid, RAPIDFIRE_DID_START, strlen( RAPIDFIRE_DID_START ) );
				break;
			case PSP_CTRL_SELECT:
				filehWrite( fuid, RAPIDFIRE_DID_SELECT, strlen( RAPIDFIRE_DID_SELECT ) );
				break;
		}
		filehWritef( fuid, RAPIDFIRE_DATA_RECORD_MAXLEN, "%s%d\n", RAPIDFIRE_DATA_RECORD_SEPARATOR, st_rfconf[i].mode );
	}
	filehWritef( fuid, RAPIDFIRE_DATA_RECORD_MAXLEN, "%s%s%d\n", RAPIDFIRE_DID_RDELAY, RAPIDFIRE_DATA_RECORD_SEPARATOR, st_release_delay );
	filehWritef( fuid, RAPIDFIRE_DATA_RECORD_MAXLEN, "%s%s%d\n", RAPIDFIRE_DID_PDELAY, RAPIDFIRE_DATA_RECORD_SEPARATOR, st_press_delay );
	
	filehClose( fuid );
	
	goto DESTROY;
	
	DESTROY:
		memsceFree( path );
		return;
}

void rapidfireInit( void )
{
	st_wait_cnt = 0;
}

void rapidfireMain( MfCallMode mode, SceCtrlData *pad_data, void *argp )
{
	int i;
	
	if( st_wait_cnt && mode == MF_CALL_READ ) st_wait_cnt--;
	
	for( i = 0; i < ARRAY_NUM( st_rfconf ); i++ ){
		if(
			(pad_data->Buttons & st_rfconf[i].button && st_rfconf[i].mode == RAPIDFIRE_MODE_SEMI_RAPID ) ||
			st_rfconf[i].mode == RAPIDFIRE_MODE_AUTO_RAPID
		 ){
			pad_data->Buttons ^= st_rapid ? st_rfconf[i].button : 0;
		} else if( st_rfconf[i].mode == RAPIDFIRE_MODE_AUTO_HOLD ){
			pad_data->Buttons |= st_rfconf[i].button;
		}
	}
	
	if( ! st_wait_cnt && mode == MF_CALL_READ ){
		st_rapid = ( ! st_rapid );
		if( st_rapid ){
			st_wait_cnt = st_press_delay;
		} else{
			st_wait_cnt = st_release_delay;
		}
	}
	
	return;
}

MfMenuReturnCode rapidfireMenu( SceCtrlLatch *pad_latch, SceCtrlData *pad_data, void *argp )
{
	static int selected = 0;
	
	switch( mfMenuVertical( blitOffsetChar( 5 ), blitOffsetLine( 4 ), BLIT_SCR_WIDTH, st_rfMenuTable, ARRAY_NUM( st_rfMenuTable ), &selected ) ){
		case MR_CONTINUE:
			blitString( blitOffsetChar( 3 ), blitOffsetLine(  2 ), MENU_FGCOLOR, MENU_BGCOLOR, "Please choose a rapidfire mode per buttons." );
			blitString( blitOffsetChar( 33 ), blitOffsetLine( 22 ), MENU_FGCOLOR, MENU_BGCOLOR, "NORMAL    : standard control mode." );
			blitString( blitOffsetChar( 33 ), blitOffsetLine( 23 ), MENU_FGCOLOR, MENU_BGCOLOR, "RAPID     : hold the button to rapidfire." );
			blitString( blitOffsetChar( 33 ), blitOffsetLine( 24 ), MENU_FGCOLOR, MENU_BGCOLOR, "AUTO-RAPID: always to rapidfire." );
			blitString( blitOffsetChar( 33 ), blitOffsetLine( 25 ), MENU_FGCOLOR, MENU_BGCOLOR, "AUTO-HOLD : always to press and hold." );
			
			/* 警告が目立つように警告はSave/Loadにカーソルがのった時のみ表示 */
			if( selected == RAPIDFIRE_LOAD || selected == RAPIDFIRE_SAVE ){
				blitString( blitOffsetChar( 5 ), blitOffsetLine( 27 ), MENU_FGCOLOR, MENU_BGCOLOR, "IMPORTANT CAUTION for Save/Load\n  Verify that MemoryStick access indicator is not blinking.\n  Otherwise, will CRASH the currently running game!" );
			} else{
				blitFillBox( blitOffsetChar( 5 ), blitOffsetLine( 27 ), BLIT_SCR_WIDTH, blitMeasureLine( 3 ), MENU_BGCOLOR );
			}
			
			blitString( blitOffsetChar( 3 ), blitOffsetLine( 31 ), MENU_FGCOLOR, MENU_BGCOLOR, "\x80\x82 = Move, \x83\x81 = Change mode, \x85 = SetDelay, \x86 = Back, START = Exit" );
			break;
		case MR_ENTER:
			switch( selected ){
				case RAPIDFIRE_LOAD:
					rapidfire_load();
					st_wait_cnt = 0;
					break;
				case RAPIDFIRE_SAVE:
					rapidfire_save();
					st_wait_cnt = 0;
					break;
			}
			break;
		case MR_BACK:
			//selected = 0;
			return MR_BACK;
	}
	
	return MR_CONTINUE;
}
