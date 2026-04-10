/*=========================================================
	
	function/remap.c
	
	ボタン再割り当て。
	
=========================================================*/
#include "remap.h"

/*=========================================================
	ローカル型宣言
=========================================================*/
struct remap_params {
	DmemUID dmem;
	PadutilRemap *remapList;
	PadutilRemap *selected;
};

/*=========================================================
	ローカル関数
=========================================================*/
static bool remap_add( struct remap_params *params );
static bool remap_delete( struct remap_params *params );
static bool remap_clear( struct remap_params *params );

static void remap_menu_load( MfMenuMessage message );
static void remap_menu_save( MfMenuMessage message );
static void remap_menu_clear( MfMenuMessage message );
static int  remap_loader( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg );
static int  remap_saver( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg );
static bool remap_save( struct remap_params *params, const char *path );
static bool remap_load( struct remap_params *params, const char *path );

/*=========================================================
	ローカル変数
=========================================================*/
static struct remap_params st_params;

/*=========================================================
	関数
=========================================================*/
void *remapProc( MfMessage message )
{
	switch( message ){
		case MF_MS_INIT:
			break;
		case MF_MS_INI_LOAD:
			return remapIniLoad;
		case MF_MS_INI_CREATE:
			return remapIniCreate;
		case MF_MS_TERM:
			break;
		case MF_MS_HOOK:
			return remapMain;
		case MF_MS_MENU:
			return remapMenu;
		default:
			break;
	}
	return NULL;
}

void remapIniLoad( IniUID ini, char *buf, size_t len )
{
	const char *section = mfGetIniTargetSection();
	
	if( inimgrGetString( ini, section, "Remap", buf, len ) > 0 ){
		remap_load( &st_params, (const char *)buf );
	}
}

void remapIniCreate( IniUID ini, char *buf, size_t len )
{
	inimgrSetString( ini, MF_INI_SECTION_DEFAULT, "Remap", "" );
}

void remapMain( MfHookAction action, SceCtrlData *pad, MfHprmKey *hk )
{
	if( st_params.remapList ){
		padutilRemap( st_params.remapList, padutilSetPad( pad->Buttons | padutilGetAnalogStickDirection( pad->Lx, pad->Ly, 0 ) ) | padutilSetHprm( *hk ), pad, hk, false );
	}
}

void remapMenu( MfMenuMessage message )
{
	static MfMenuTable *menu;
	static char column;
	
	switch( message ){
		case MF_MM_INIT:
			dbgprint( "Creating remap menu" );
			menu = mfMenuCreateTables(
				1,
				4, 1
			);
			if( ! menu ) return;
			
			mfMenuSetTablePosition( menu, 1, gbOffsetChar( 57 ), gbOffsetLine( 7 ) );
			mfMenuSetTableEntry( menu, 1, 1, 1, "Load", mfCtrlDefExtra, remap_menu_load, NULL );
			mfMenuSetTableEntry( menu, 1, 2, 1, "Save", mfCtrlDefExtra, remap_menu_save, NULL );
			mfMenuSetTableEntry( menu, 1, 4, 1, "Clear", mfCtrlDefExtra, remap_menu_clear, NULL );
			mfMenuInitTable( menu );
			break;
		case MF_MM_TERM:
			mfMenuDestroyTables( menu );
			return;
		default:
		{
			uint8_t viewlines = 0;
			unsigned int buttons = mfMenuGetCurrentButtons();
			
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine(  4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Remapping buttons list." );
			
			if( ! st_params.remapList ){
				mfMenuInactiveTableEntry( menu, 1, 2, 1 );
				gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 7 ), MF_COLOR_TEXT_FC, MF_COLOR_TEXT_BG, "<Press TRIANGLE(\x84) to add a new remap>" );
			} else{
				PadutilRemap *entry;
				char         real_buttons[64];
				char         remap_buttons[64];
				unsigned int pos, maxpos, line, skip;
				uint8_t      linepos;
				
				mfMenuActiveTableEntry( menu, 1, 2, 1 );
				
				/* リマップされている個数と選択中のエントリの位置を数える */
				for( entry = st_params.remapList, pos = 0, maxpos = 0; entry; entry = entry->next, maxpos++ ){
					if( entry == st_params.selected ) pos = maxpos;
				}
				maxpos--;
				
				/* スクロール */
				skip = mfMenuScroll( pos, REMAP_LINES_PER_PAGE, maxpos );
				for( entry = st_params.remapList, line = 0; skip; entry = entry->next, line++, skip-- );
				
				/* 表示 */
				for( linepos = 0, viewlines = REMAP_LINES_PER_PAGE; viewlines && entry; entry = entry->next, line++, viewlines--, linepos++ ){
					if( ! entry->realButtons ) continue;
					
					real_buttons[0]  = '\0';
					//remap_buttons[0] = '\0';
					
					mfMenuButtonsSymbolList( entry->realButtons,  real_buttons,  sizeof( real_buttons ) );
					//mfMenuButtonsSymbolList( entry->remapButtons, remap_buttons, sizeof( remap_buttons ) );
					if( pos == line ){
						//gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 7 + linepos ), MF_COLOR_TEXT_FC, MF_COLOR_TEXT_BG, "%s: %s", real_buttons, remap_buttons );
						gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 7 + linepos ), MF_COLOR_TEXT_FC, MF_COLOR_TEXT_BG, real_buttons );
					} else{
						//gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 7 + linepos ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "%s: %s", real_buttons, remap_buttons );
						gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 7 + linepos ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, real_buttons );
					}
				}
							
				remap_buttons[0] = '\0';
				mfMenuButtonsSymbolList( st_params.selected->remapButtons, remap_buttons, sizeof( remap_buttons ) );
				gbLineRectRel( gbOffsetChar( 15 ), gbOffsetLine( 6 + REMAP_LINES_PER_PAGE + 3 ), gbOffsetChar( 64 ), gbOffsetLine( 3 ), MF_COLOR_TEXT_FG );
				gbPrint( gbOffsetChar( 2 ), gbOffsetLine( 6 + REMAP_LINES_PER_PAGE + 4 ), MF_COLOR_TEXT_FG, MF_COLOR_TEXT_BG, "Remapping to" );
				gbPrint( gbOffsetChar( 16 ), gbOffsetLine( 6 + REMAP_LINES_PER_PAGE + 4 ), MF_COLOR_TEXT_FC, MF_COLOR_TEXT_BG, remap_buttons[0] != '\0' ? remap_buttons : "Disable" );
			}
			
			if( column == REMAP_COLUMN_LIST ){
				gbLineRectRel( gbOffsetChar(  2 ), gbOffsetLine( 6 ), gbOffsetChar( 53 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MF_COLOR_TEXT_FC );
				gbLineRectRel( gbOffsetChar( 56 ), gbOffsetLine( 6 ), gbOffsetChar( 23 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MF_COLOR_TEXT_FG );
				
				mfMenuDrawTable( menu, MF_MENU_DISPLAY_ONLY );
				
				if( mfDialogCurrentType() == MFDIALOG_DETECTBUTTONS ){
					if( ! mfDialogDetectbuttonsDraw() ){
						if( ! mfDialogDetectbuttonsResult() && ! st_params.selected->remapButtons ){
							remap_delete( &st_params );
						}
						mfMenuResetKeyRepeat();
					}
				} else{
					if( ! st_params.remapList ){
						mfMenuSetInfoText( MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_MOVABLEPAGE_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(L/R)Switch column, (\x84)Add" );
					} else{
						mfMenuSetInfoText( MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_MOVABLEPAGE_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(L/R)Switch column, (\x84)Add, (\x87)Delete, (\x85)Edit remap buttons" );
					}
					if( buttons ){
						if( buttons & PSP_CTRL_CROSS ){
							mfMenuProc( NULL );
						} else if( buttons & PSP_CTRL_TRIANGLE ){
							if( remap_add( &st_params ) ){
								mfDialogDetectbuttonsInit( "Set real buttons", MF_HOTKEY_BUTTONS, &(st_params.selected->realButtons) );
							}
						} else if( buttons & ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER ) ){
							column = REMAP_COLUMN_MENU;
						} else if( st_params.selected ){
							if( buttons & PSP_CTRL_CIRCLE ){
								mfDialogDetectbuttonsInit( "Set remap buttons", MF_TARGET_BUTTONS, &(st_params.selected->remapButtons) );
							} else if( buttons & PSP_CTRL_SQUARE ){
								remap_delete( &st_params );
							} else if( buttons & PSP_CTRL_UP ){
								if( st_params.selected->prev ){
									st_params.selected = st_params.selected->prev;
								} else{
									for( ; st_params.selected->next; st_params.selected = st_params.selected->next );
								}
							} else if( buttons & PSP_CTRL_DOWN ){
								if( st_params.selected->next ){
									st_params.selected = st_params.selected->next;
								} else{
									st_params.selected = st_params.remapList;
								}
							} else if( buttons & PSP_CTRL_RIGHT ){
								for( viewlines = REMAP_LINES_PER_PAGE; viewlines && st_params.selected->next; viewlines--, st_params.selected = st_params.selected->next );
							} else if( buttons & PSP_CTRL_LEFT ){
								for( viewlines = REMAP_LINES_PER_PAGE; viewlines && st_params.selected->prev; viewlines--, st_params.selected = st_params.selected->prev );
							}
						}
					}
				}
			} else{
				gbLineRectRel( gbOffsetChar(  2 ), gbOffsetLine( 6 ), gbOffsetChar( 53 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MF_COLOR_TEXT_FG );
				gbLineRectRel( gbOffsetChar( 56 ), gbOffsetLine( 6 ), gbOffsetChar( 23 ), gbOffsetLine( REMAP_LINES_PER_PAGE + 2 ), MF_COLOR_TEXT_FC );
				
				if( message != MF_MM_EXTRA ){
					mfMenuSetInfoText( MF_MENU_INFOTEXT_COMMON_CTRL | MF_MENU_INFOTEXT_SET_LOWER_LINE, "(L/R)Switch column" );
					
					if( buttons & ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER ) ){
						column = REMAP_COLUMN_LIST;
					}
				}
				if( ! mfMenuDrawTable( menu, MF_MENU_NO_OPTIONS ) ) mfMenuProc( NULL );
			}
		}
	}
}

static bool remap_add( struct remap_params *params )
{
	PadutilRemap *new_entry;
	PadutilRemap *last = NULL;
	
	if( ! params->remapList ){
		dbgprint( "New entry" );
		params->dmem      = dmemNew( 512, PSP_SMEM_Low );
		params->remapList = dmemAlloc( params->dmem, sizeof( PadutilRemap ) );
		new_entry         = params->remapList;
	} else{
		dbgprint( "Add entry" );
		
		/* 末尾まで移動 */
		for( last = params->remapList; last->next; last = last->next );
		
		last->next = dmemAlloc( params->dmem, sizeof( PadutilRemap ) );
		new_entry = last->next;
	}
	
	if( new_entry ){
		dbgprintf( "Init entry: %p", new_entry );
		new_entry->realButtons  = 0;
		new_entry->remapButtons = 0;
		new_entry->prev         = last;
		new_entry->next         = NULL;
		
		params->selected = new_entry;
		
		return true;
	} else{
		return false;
	}
}

static bool remap_delete( struct remap_params *params )
{
	if( params->selected ){
		PadutilRemap *target = params->selected;
		if( target->next ){
			target->next->prev = target->prev;
			params->selected = target->next;
		} else{
			params->selected = target->prev;
		}
		if( target->prev ) target->prev->next = target->next;
		
		if( target == params->remapList ){
			params->remapList = params->selected;
			if( ! params->remapList ){
				dmemDestroy( params->dmem );
				params->selected = NULL;
			} else{
				dmemFree( params->dmem, target );
			}
		}
	}
	return true;
}

static bool remap_clear( struct remap_params *params )
{
	if( params->selected ){
		dmemDestroy( params->dmem );
		params->remapList = NULL;
		params->selected  = NULL;
	}
	return true;
}

static bool remap_save( struct remap_params *params, const char *path )
{
	IniUID ini = inimgrNew();
	
	if( ini ){
		int ret;
		
		inimgrAddSection( ini, REMAP_SECTION_DATA );
		inimgrSetCallback( ini, REMAP_SECTION_DATA, remap_saver, (void *)params );
		ret = inimgrSave( ini, path, REMAP_INI_SIGNATURE, REMAP_INI_VERSION, 0, 0 );
		
		if( ret != 0 ){
			mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: %X", ret );
			mfMenuWait( MF_ERROR_DELAY );
		} else{
			inimgrDestroy( ini );
			return true;
		}
		inimgrDestroy( ini );
	} else{
		mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: Not enough memory." );
		mfMenuWait( MF_ERROR_DELAY );
	}
	return false;
}

static bool remap_load( struct remap_params *params, const char *path )
{
	IniUID ini = inimgrNew();
	
	if( ini ){
		int ret;
		
		remap_clear( params );
		
		inimgrSetCallback( ini, REMAP_SECTION_DATA, remap_loader, (void *)params );
		ret = inimgrLoad( ini, path, REMAP_INI_SIGNATURE, REMAP_INI_VERSION, 0, 0 );
		
		if( ret != 0 ){
			if( ret == INIMGR_ERROR_INVALID_SIGNATURE ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Failure: Not a MacroFire remap file." );
			} else if( ret == INIMGR_ERROR_INVALID_VERSION ){
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Failure: Unsupported file version." );
			} else {
				mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: %X", ret );
			}
			mfMenuWait( MF_ERROR_DELAY );
		} else{
			inimgrDestroy( ini );
			return true;
		}
		inimgrDestroy( ini );
	} else{
		mfMenuSetInfoText( MF_MENU_INFOTEXT_ERROR | MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "inimgr Error: Not enough memory." );
		mfMenuWait( MF_ERROR_DELAY );
	}
	return false;
}

/*-----------------------------------------------
	セーブロードコールバック
-----------------------------------------------*/
static int remap_loader( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg )
{
	struct remap_params *params = (struct remap_params *)arg;
	char *name, *value;
	bool update = false;
	
	dbgprint( "Remap loader called." );
	
	if( ! mfConvertButtonReady() ){
		return CG_ERROR_NOT_ENOUGH_MEMORY;
	}
	
	if( ! remap_add( params ) ){
		mfConvertButtonFinish();
		return CG_ERROR_NOT_ENOUGH_MEMORY;
	}
	
	while( inimgrCbReadln( cbp, buf, buflen ) ){
		strutilRemoveChar( buf, "\x20\t" );
		if( ! inimgrParseEntry( buf, &name, &value ) ) continue;
		
		if( strcasecmp( name, "RealButtons" ) == 0 ){
			if( ! params->selected->realButtons ){
				params->selected->realButtons = mfConvertButtonN2C( value );
				update = true;
			}
		} else if( strcasecmp( name, "RemapButtons" ) == 0 ){
			if( ! params->selected->remapButtons ){
				params->selected->remapButtons = mfConvertButtonN2C( value );
				update = true;
			}
		}
	}
	
	if( ! update ) remap_delete( params );
	mfConvertButtonFinish();
	
	return 0;
}

static int remap_saver( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg )
{
	struct remap_params *params = (struct remap_params *)arg;
	char section[INIMGR_SECTION_BUFFER];
	char tmpbuf[128];
	unsigned int len, seq;
	PadutilRemap *remap;
	
	inimgrDeleteSection( inimgrCbGetIniHandle( cbp ), REMAP_SECTION_DATA );
	
	if( ! mfConvertButtonReady() ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	for( seq = 1, remap = params->remapList; remap; remap = remap->next, seq++ ){
		if( ! remap->realButtons ) continue;
		
		/* 見やすいように空行書き込み */
		inimgrCbWriteln( cbp, buf, 0 );
		
		/* セクション名書き込み */
		strutilNCopy( tmpbuf, REMAP_SECTION_DATA, strlen( REMAP_SECTION_DATA ) - 1, sizeof( tmpbuf ) );
		snprintf( section, sizeof( section ), "%s%d", tmpbuf, seq );
		len = inimgrMakeSection( buf, buflen, section );
		inimgrCbWriteln( cbp, buf, len );
		
		/* データ書き込み */
		mfConvertButtonC2N( remap->realButtons, tmpbuf, sizeof( tmpbuf ) );
		len = inimgrMakeEntry( buf, buflen, "RealButtons", tmpbuf );
		inimgrCbWriteln( cbp, buf, len );
		mfConvertButtonC2N( remap->remapButtons, tmpbuf, sizeof( tmpbuf ) );
		len = inimgrMakeEntry( buf, buflen, "RemapButtons", tmpbuf );
		inimgrCbWriteln( cbp, buf, len );
	}
	
	mfConvertButtonFinish();
	
	return 0;
}

/*-----------------------------------------------
	メニューコントロール
-----------------------------------------------*/
static void remap_menu_save( MfMenuMessage message )
{
	static char *path;
	
	if( message == MF_MM_INIT ){
		path = memoryAllocEx( "RemapSave", MEMORY_USER, 0, MF_PATH_MAX, PSP_SMEM_High, NULL );
		
		if( ! mfDialogGetfilenameInit( "Save remap preference", "ms0:", "remap.ini", path, 255, CDIALOG_GETFILENAME_SAVE | CDIALOG_GETFILENAME_OVERWRITEPROMPT  ) ){
			memoryFree( path );
			path = NULL;
			mfMenuExitExtra();
		}
	} else if( message == MF_MM_PROC ){
		if( mfDialogCurrentType() ){
			if( ! mfDialogGetfilenameDraw() ){
				if( ! mfDialogGetfilenameResult() ){
					memoryFree( path );
					path = NULL;
				} else{
					mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Saving %s...", path );
				}
			}
		} else if( path ){
			remap_save( &st_params, path );
			memoryFree( path );
			path = NULL;
		} else if( ! path ){
			mfMenuExitExtra();
		}
	}
}

static void remap_menu_load( MfMenuMessage message )
{
	static char *path;
	
	if( message == MF_MM_INIT ){
		path = memoryAllocEx( "RemapLoad", MEMORY_USER, 0, MF_PATH_MAX, PSP_SMEM_High, NULL );
		
		if( ! mfDialogGetfilenameInit( "Load remap preference", "ms0:", "", path, 255, CDIALOG_GETFILENAME_OPEN | CDIALOG_GETFILENAME_FILEMUSTEXIST ) ){
			memoryFree( path );
			path = NULL;
			mfMenuExitExtra();
		}
	} else if( message == MF_MM_PROC ){
		if( mfDialogCurrentType() ){
			if( ! mfDialogGetfilenameDraw() ){
				if( ! mfDialogGetfilenameResult() ){
					memoryFree( path );
					path = NULL;
				} else{
					mfMenuSetInfoText( MF_MENU_INFOTEXT_SET_MIDDLE_LINE, "Loading %s...", path );
				}
			}
		} else if( path ){
			if( ! remap_load( &st_params, path ) ){
				remap_clear( &st_params );
			}
			memoryFree( path );
			path = NULL;
		} else if( ! path ){
			mfMenuExitExtra();
		}
	}
}

static void remap_menu_clear( MfMenuMessage message )
{
	if( message == MF_MM_INIT ){
		if( ! mfDialogMessageInit( "Warning", "Clearing all remap entries.\nAre you sure?", true ) ) return;
	} else if( message == MF_MM_PROC ){
		if( ! mfDialogMessageDraw() ){
			if( mfDialogMessageResult() ){
				remap_clear( &st_params );
			}
			mfMenuExitExtra();
		}
	}
}

