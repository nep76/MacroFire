/*
	Macro
*/

#include "macro.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
/* プロシージャ */
static MfMenuRc macro_slot_proc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg );

/* メニュー処理 */
static MfMenuRc macro_menu_halt( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_run( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_load( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_save( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_edit( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_clear( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_record_start( SceCtrlData *pad_data, void *arg );
static MfMenuRc macro_menu_record_stop( SceCtrlData *pad_data, void *arg );

/* 適当なロック */
static bool macro_lock( void );
static void macro_unlock( void );

/* マクロ操作 */
static bool macro_select( unsigned int slot );
static bool macro_create( MacroEntry *entry );
static bool macro_run( MacroEntry *entry, unsigned int loop );
static bool macro_purge( MacroEntry *entry );
static bool macro_stop( MacroEntry *entry );
static void macro_init_tempdata( struct macro_tempdata *temp );
static void macro_record( MfCallMode mode, SceCtrlData *pad_data, void *argp );
static void macro_trace( MfCallMode mode, SceCtrlData *pad_data, void *argp );
static void macro_load( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen );
static void macro_save( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen );

/* エラーチェック */
static bool macro_in_webbrowser( void );
static bool macro_is_busy( MacroEntry *entry );
static bool macro_is_unavail( MacroEntry *entry );
static bool macro_is_disable_engine( void );
static bool macro_is_locked( void );
static bool macro_is_busy_with_message( MacroEntry *entry );
static bool macro_is_unavail_with_message( MacroEntry *entry );
static bool macro_is_disable_engine_with_message( void );
static bool macro_is_locked_with_message( void );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static bool           st_exlock = false;
static MfMenuCallback st_callback;

static unsigned int st_slot = 0;
static MacroEntry   st_macro[MACRO_MAX_SLOT];

static MfMenuItem st_menu_table[] = {
	{ "Macro slot: %d %s", macro_slot_proc, &st_slot, { { .string = "Select macro slot" }, { .string = "slot" }, { .integer = 1 }, { .integer = 1 }, { .integer = MACRO_MAX_SLOT - 1 } } },
	{ NULL },
	{ "Buttons to run the macro", mfMenuDefGetButtonsProc, NULL, { { .string = "Trigger buttons" }, { .integer = MFM_ALL_AVAILABLE_BUTTONS } } },
	{ NULL },
	{ "Run once",     mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_run  }, { .integer = 1    } } },
	{ "Run infinity", mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_run  }, { .integer = -1   } } },
	{ "Stop macro",   mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_halt }, { .pointer = NULL } } },
	{ NULL },
	{ "Start recording",            mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_record_start }, { .pointer = NULL } } },
	{ "Stop recording",             mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_record_stop  }, { .pointer = NULL } } },
	{ "Recording analog stick: %s", mfMenuDefBoolProc,     NULL,         { { .string = "OFF" }, { .string = "ON" } } },
	{ NULL },
	{ "Edit macro",  mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_edit  }, { .pointer = NULL } } },
	{ "Clear macro", mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_clear }, { .pointer = NULL } } },
	{ NULL },
	{ "Load from MemoryStick", mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_load }, { .pointer = NULL } } },
	{ "Save to MemoryStick",   mfMenuDefCallbackProc, &st_callback, { { .pointer = macro_menu_save }, { .pointer = NULL } } }
};

/*=============================================*/

void macroInit( void )
{
	macro_select( 0 );
	return;
}

void macroTerm( void )
{
	int i;
	for( i = 0; i < MACRO_MAX_SLOT; i++ ){
		macro_purge( &(st_macro[i]) );
	}
}

void macroIntr( const bool mfengine )
{
	if( ! mfengine ){
		int i;
		for( i = 0; i < MACRO_MAX_SLOT; i++ ){
			macro_stop( &(st_macro[i]) );
		}
	}
}

void macroMain( MfCallMode mode, SceCtrlData *pad, void *argp )
{
	int i;
	if( ! pad || macro_in_webbrowser() ) return;
	
	for( i = 0; i < MACRO_MAX_SLOT; i++ ){
		if( ! st_macro[i].current.macro ) continue;
		
		if( st_macro[i].current.runMode == MRM_TRACE ){
			macro_trace( mode, pad, &(st_macro[i]) );
		} else if( st_macro[i].current.runMode == MRM_RECORD ){
			macro_record( mode, pad, &(st_macro[i]) );
		}
		
		if( st_macro[i].runButtons ){
			bool trigger = ( ( ( pad->Buttons | ctrlpadUtilGetAnalogDirection( pad->Lx, pad->Ly, 0 ) ) & st_macro[i].runButtons ) == st_macro[i].runButtons ) ? true : false;
			
			if( trigger && st_macro[i].current.hotkeyEnable ){
				if( st_macro[i].current.runMode == MRM_NONE ){
					macro_run( &(st_macro[i]), 1 );
				} else if( st_macro[i].current.runMode == MRM_TRACE ){
					macro_stop( &(st_macro[i]) );
				}
				st_macro[i].current.hotkeyEnable = false;
			} else if( ! trigger && ! st_macro[i].current.hotkeyEnable ){
				st_macro[i].current.hotkeyEnable = true;
			}
		}
	}
}

MfMenuRc macroMenu( SceCtrlData *pad_data, void *arg )
{
	static int selected = 0;
	
	if( macro_in_webbrowser() ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Cannot use this function on the web-browser." );
		mfMenuWait( 1000000 );
		return MR_BACK;
	}
	
	if( ! st_callback.func ){
		switch( mfMenuUniDraw( gbOffsetChar( 5 ), gbOffsetLine( 6 ), st_menu_table, MF_ARRAY_NUM( st_menu_table ), &selected, 0 ) ){
			case MR_CONTINUE:
				gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Please choose a operation." );
				switch( selected ){
					case MACRO_SELECT_SLOT  : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Macro slot number." ); break;
					case MACRO_SET_TRIGGER  : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Buttons to run the macro." ); break;
					case MACRO_RUN_ONCE     : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Running currently macro for once." ); break;
					case MACRO_RUN_INFINITY : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Running currently macro for endless." ); break;
					case MACRO_RUN_HALT     : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Stopping currently running macro." ); break;
					case MACRO_RECORD_START : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Starting to record a macro." ); break;
					case MACRO_RECORD_STOP  : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Stopping to record a macro." ); break;
					case MACRO_ANALOG_OPTION: gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Recording the analog stick movement." ); break;
					case MACRO_EDIT         : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Editing currently macro." ); break;
					case MACRO_CLEAR        : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Clearing currently macro." ); break;
					case MACRO_LOAD         : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Loading a macro from MemoryStick." ); break;
					case MACRO_SAVE         : gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 25 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Saving currently macro to MemoryStick." ); break;
				}
				break;
			case MR_ENTER:
				break;
			case MR_BACK:
				return MR_BACK;
		}
	} else{
		if( ( (MacroFunction)(st_callback.func) )( pad_data, st_callback.arg ) == MR_BACK ){
			st_callback.func = NULL;
		}
	}
	return MR_CONTINUE;
}

/*-------------------------------------
	スタティック関数
--------------------------------------*/
static bool macro_in_webbrowser( void )
{
	if( sceKernelFindModuleByName( "sceHVNetfront_Module" ) != NULL ){
		return true;
	} else{
		return false;
	}
}

static bool macro_lock( void )
{
	if( ! st_exlock ){
		st_exlock = true;
		return true;
	}
	return false;
}

static void macro_unlock( void )
{
	st_exlock = false;
}

static bool macro_select( unsigned int slot )
{
	if( slot < MACRO_MAX_SLOT ){
		st_slot = slot;
		st_menu_table[MACRO_SET_TRIGGER].var   = &(st_macro[slot].runButtons);
		st_menu_table[MACRO_ANALOG_OPTION].var = &(st_macro[slot].analogStick);
		return true;
	}
	return false;
}

static bool macro_run( MacroEntry *entry, unsigned int loop )
{
	if( macro_is_unavail( entry ) || macro_is_busy( entry ) || macro_is_disable_engine() || macro_is_locked() ) return false;
	
	/* マクロ実行準備 */
	entry->current.runMode = MRM_TRACE;
	entry->current.runLoop = loop;
	entry->current.rfUid   = mfRapidfireNew();
	entry->current.command = NULL;
	
	/* 最後のボタン情報をリセット */
	macro_init_tempdata( &(entry->current.temp) );
	sceRtcGetCurrentTick( &(entry->current.temp.rtc) );
	
	macro_lock();
	
	return true;
}

static bool macro_create( MacroEntry *entry )
{
	if( ! macro_is_unavail( entry ) ) return false;
	
	entry->current.macro = macromgrNew();
	if( ! entry->current.macro ) return false;
	
	entry->current.command = NULL;
	entry->current.runMode = MRM_NONE;
	entry->current.runLoop = 0;
	entry->current.rfUid   = 0;
	
	return true;
}

static bool macro_purge( MacroEntry *entry )
{
	if( macro_is_unavail( entry ) ) return false;
	
	macro_stop( entry );
	
	macromgrDestroy( entry->current.macro );
	entry->current.macro = NULL;
	
	return true;
}

static bool macro_stop( MacroEntry *entry )
{
	if( entry->current.runMode != MRM_TRACE ) return false;
	
	entry->current.runMode = MRM_NONE;
	
	if( entry->current.rfUid ){
		mfRapidfireDestroy( entry->current.rfUid );
	}
	
	macro_unlock();
	
	return true;
}

static void macro_init_tempdata( struct macro_tempdata *temp )
{
	temp->buttons     = 0;
	temp->analogCoord = MACROMGR_ANALOG_NEUTRAL;
	temp->rtc         = 0;
}

/* スロット切り替え時に処理を追加 */
static MfMenuRc macro_slot_proc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	MfMenuRc rc;
	
	if( signal == MS_QUERY_LABEL ){
		char *macro_stat;
		if( ! st_macro[*((unsigned int *)var)].current.macro ){
			macro_stat = "";
		} else if( st_macro[*((unsigned int *)var)].current.runMode == MRM_TRACE ){
			macro_stat = "(Running)";
		} else if( st_macro[*((unsigned int *)var)].current.runMode == MRM_RECORD ){
			macro_stat = "(Recording)";
		} else{
			macro_stat = "(Available)";
		}
		mfMenuLabel( (char *)arg, *((unsigned int *)var) + 1, macro_stat );
		rc = MR_CONTINUE;
	} else{
		rc = mfMenuDefGetNumberProc( signal, pad, var, value, arg );
	}
	
	if( pad->Buttons & ( PSP_CTRL_LEFT | PSP_CTRL_RIGHT ) ) macro_select( *((unsigned int *)var) );
	
	return rc;
}

static MfMenuRc macro_menu_halt( SceCtrlData *pad_data, void *arg )
{
	if( macro_stop( &(st_macro[st_slot]) ) ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Macro halted." );
	} else{
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Not ran yet." );
	}
	mfMenuWait( 1000000 );
	return MR_BACK;
}

static MfMenuRc macro_menu_run( SceCtrlData *pad_data, void *arg )
{
	int loop = MACRO_GET_ARG_BY_INT( arg );
	
	if(
		macro_is_busy_with_message( &(st_macro[st_slot]) )    ||
		macro_is_unavail_with_message( &(st_macro[st_slot]) ) ||
		macro_is_disable_engine_with_message()                ||
		macro_is_locked_with_message()
	) return MR_BACK;
	
	macro_run( &(st_macro[st_slot]), loop );
	
	if( loop == 1 ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Run once..." );
	} else if( loop == -1 ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Run infinity..." );
	} else{
		gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Run %d times...", loop );
	}
		
	mfMenuWait( 1000000 );
	mfMenuQuit();
	
	return MR_BACK;
}


static MfMenuRc macro_menu_load( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy_with_message( &(st_macro[st_slot]) ) ) return MR_BACK;
	
	if( ! mfMenuGetFilenameIsReady() ){
		if( mfMenuGetFilenameInit( "Open macro", CGF_OPEN | CGF_FILEMUSTEXIST, "ms0:/", 128, 128 ) ){
			return MR_CONTINUE;
		} else{
			return MR_BACK;
		}
	} else{
		if( ! mfMenuGetFilename() ){
			char inipath[128 + 128];
			inipath[0] = '\0';
			
			if( mfMenuGetFilenameResult( inipath, sizeof( inipath ), NULL ) ){
				IniUID ini = inimgrNew();
				if( ini > 0 ){
					unsigned int err;
					inimgrSetCallback( ini, MACRO_DATA_SECTION, macro_load );
					if( ( err = inimgrLoad( ini, inipath ) ) == 0 ){
						int version;
						if( ! inimgrGetInt( ini, "default", MACRO_DATA_SIGNATURE, (long *)&version ) || version <= MACRO_DATA_REFUSE_VERSION ){
							gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Macro file is invalid or too lower version." );
							mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
						} else{
							gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Loaded from %s.", inipath );
							mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
						}
					} else{
						gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Failed to load from %s: 0x%X", inipath, err );
						mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
					}
					inimgrDestroy( ini );
				}
			}
			return MR_BACK;
		}
		return MR_CONTINUE;
	}
}

static MfMenuRc macro_menu_save( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy_with_message( &(st_macro[st_slot]) ) || macro_is_unavail_with_message( &(st_macro[st_slot]) ) ) return MR_BACK;
	
	if( ! mfMenuGetFilenameIsReady() ){
		if( mfMenuGetFilenameInit( "Save macro", CGF_SAVE | CGF_FILEMUSTEXIST, "ms0:/", 128, 128 ) ){
			return MR_CONTINUE;
		} else{
			return MR_BACK;
		}
	} else{
		if( ! mfMenuGetFilename() ){
			char inipath[128 + 128];
			inipath[0] = '\0';
			
			if( mfMenuGetFilenameResult( inipath, sizeof( inipath ), NULL ) ){
				IniUID ini = inimgrNew();
				if( ini > 0 ){
					unsigned int err;
					inimgrSetInt( ini, "default", MACRO_DATA_SIGNATURE, MACRO_DATA_VERSION );
					inimgrAddSection(  ini, MACRO_DATA_SECTION );
					inimgrSetCallback( ini, MACRO_DATA_SECTION, macro_save );
					if( ( err = inimgrSave( ini, inipath ) ) == 0 ){
						gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FGCOLOR, "Saved to %s.", inipath );
						mfMenuWait( MFM_DISPLAY_MICROSEC_INFO );
					} else{
						gbPrintf( gbOffsetChar( 3 ), gbOffsetLine( 32 ), MFM_TEXT_BGCOLOR, MFM_TEXT_FCCOLOR, "Failed to save to %s: 0x%X", inipath, err );
						mfMenuWait( MFM_DISPLAY_MICROSEC_ERROR );
					}
					inimgrDestroy( ini );
				}
			}
			return MR_BACK;
		}
		return MR_CONTINUE;
	}
}

static MfMenuRc macro_menu_edit( SceCtrlData *pad_data, void *arg )
{
	MfMenuRc rc;
	
	if( macro_is_busy_with_message( &(st_macro[st_slot]) ) ) return MR_BACK;
	
	if( ! st_macro[st_slot].current.macro ){
		if( ! macro_create( &(st_macro[st_slot]) ) ){
			gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Failed to create." );
			mfMenuWait( 1000000 );
			return MR_BACK;
		}
		
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Creating new macro..." );
		mfMenuWait( 1000000 );
		return MR_CONTINUE;
	}
	
	mfMenuDisableInterrupt();
	
	rc = macroeditorMain( pad_data, st_macro[st_slot].current.macro );
	
	if( rc == MR_BACK ) mfMenuEnableInterrupt();
	return rc;
}

static MfMenuRc macro_menu_clear( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy_with_message( &(st_macro[st_slot]) ) || macro_is_unavail_with_message( &(st_macro[st_slot]) ) ) return MR_BACK;
	
	macro_purge( &(st_macro[st_slot]) );
	
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Clearing currently macro..." );
	mfMenuWait( 1000000 );
	
	return MR_BACK;
}

static MfMenuRc macro_menu_record_start( SceCtrlData *pad_data, void *arg )
{
	if( macro_is_busy_with_message( &(st_macro[st_slot]) ) || macro_is_disable_engine_with_message() || macro_is_locked_with_message() ) return MR_BACK;
	
	if( st_macro[st_slot].current.runMode == MRM_RECORD ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Already started recording" );
		mfMenuWait( 1000000 );
		return MR_BACK;
	}
	
	/* 既にマクロがあれば削除 */
	if( st_macro[st_slot].current.macro ) macro_purge( &(st_macro[st_slot]) );
	
	/* マクロを初期化 */
	if( ! macro_create( &(st_macro[st_slot]) ) ) return MR_BACK;
	
	/* 最後のボタン情報をリセット */
	macro_init_tempdata( &(st_macro[st_slot].current.temp) );
	
	/* 記録の準備 */
	st_macro[st_slot].current.runMode         = MRM_RECORD;
	st_macro[st_slot].current.command         = st_macro[st_slot].current.macro;
	st_macro[st_slot].current.command->action = MA_DELAY;
	st_macro[st_slot].current.command->data   = 0;
	st_macro[st_slot].current.command->sub    = 0;
	sceRtcGetCurrentTick( &(st_macro[st_slot].current.temp.rtc) );
	
	mfMenuQuit();
	
	macro_lock();
	
	return MR_BACK;
}

static MfMenuRc macro_menu_record_stop( SceCtrlData *pad_data, void *arg )
{
	if( st_macro[st_slot].current.runMode == MRM_RECORD ){
		macro_unlock();
		st_macro[st_slot].current.runMode = MRM_NONE;
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Stopping macro recording..." );
	} else{
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Not recorded yet." );
	}
	
	mfMenuWait( 1000000 );
	return MR_BACK;
}

static void macro_update_delayms( MacroData *macro, uint64_t origin )
{
	uint64_t rtc, waitms;
	sceRtcGetCurrentTick( &rtc );
	
	waitms = ( rtc - origin ) / 1000;
	macro->data = waitms > MACROMGR_MAX_DELAY ? MACROMGR_MAX_DELAY : waitms;
}

static void macro_record( MfCallMode mode, SceCtrlData *pad, void *argp )
{
	MacroEntry   *entry;
	unsigned int current_buttons;
	uint64_t     current_analog_coord = MACROMGR_ANALOG_NEUTRAL;
	bool         analog_move          = false;
	
	if( mode == MF_CALL_LATCH || ! pad || ! argp ) return;
	
	entry           = (MacroEntry *)argp;
	current_buttons = pad->Buttons & 0x0000FFFF;
	if( entry->analogStick ){
		current_analog_coord = MACROMGR_SET_ANALOG_XY( pad->Lx, pad->Ly );
		if( current_analog_coord != entry->current.temp.analogCoord ) analog_move = true;
	}
	
	if( entry->current.temp.buttons == current_buttons && ! analog_move ){
		/* ボタンもアナログスティックも前回と同じ場合は、ディレイを更新して終了 */
		macro_update_delayms( entry->current.command, entry->current.temp.rtc );
		return;
	} else{
		/* ボタンかアナログスティックが異なる場合 */
		unsigned int press_buttons   = ( entry->current.temp.buttons ^ current_buttons ) & current_buttons;
		unsigned int release_buttons = ( entry->current.temp.buttons ^ current_buttons ) & entry->current.temp.buttons;
		
		if( entry->current.command->action == MA_DELAY && entry->current.command->data != 0 ){
			/* 現在のコマンドがディレイで時間が0より大きい場合は、ディレイを更新して次のコマンド作成 */
			macro_update_delayms( entry->current.command, entry->current.temp.rtc );
			entry->current.command = macromgrInsertAfter( entry->current.command );
		} else if( entry->current.command->action != MA_DELAY ){
			/* 現在のコマンドがディレイじゃない場合は次のコマンド作成 */
			entry->current.command = macromgrInsertAfter( entry->current.command );
		}
		
		/* コマンドの追加に失敗していれば終了 */
		if( ! entry->current.command ) return;
		
		/* ボタンが異なっていればボタン変更コマンドをセット */
		if( press_buttons && release_buttons ){
			entry->current.command->action = MA_BUTTONS_CHANGE;
			entry->current.command->data   = current_buttons;
			entry->current.command->sub    = 0;
			entry->current.command         = macromgrInsertAfter( entry->current.command );
		} else if( press_buttons ){
			entry->current.command->action = MA_BUTTONS_PRESS;
			entry->current.command->data   = press_buttons;
			entry->current.command->sub    = 0;
			entry->current.command         = macromgrInsertAfter( entry->current.command );
		} else if( release_buttons ){
			entry->current.command->action = MA_BUTTONS_RELEASE;
			entry->current.command->data   = release_buttons;
			entry->current.command->sub    = 0;
			entry->current.command         = macromgrInsertAfter( entry->current.command );
		}
		
		/* コマンドの追加に失敗していれば終了 */
		if( ! entry->current.command ) return;
		
		/* アナログスティックが動いていれば移動コマンドをセット */
		if( analog_move ){
			entry->current.command->action = MA_ANALOG_MOVE;
			entry->current.command->data   = current_analog_coord;
			entry->current.command->sub    = 0;
			entry->current.command         = macromgrInsertAfter( entry->current.command );
		}
		
		/* コマンドの追加に失敗していれば終了 */
		if( ! entry->current.command ) return;
		
		/* ディレイコマンドをセット */
		entry->current.command->action = MA_DELAY;
		entry->current.command->data   = 0;
		entry->current.command->sub    = 0;
		
		/* 一時データを更新する */
		entry->current.temp.buttons     = current_buttons;
		entry->current.temp.analogCoord = current_analog_coord;
		sceRtcGetCurrentTick( &(entry->current.temp.rtc) );
	}
}

static void macro_trace( MfCallMode mode, SceCtrlData *pad, void *argp )
{
	MacroEntry *entry;
	int coord;
	
	if( ! pad || ! argp ) return;
	
	entry = (MacroEntry *)argp;
	
	if( mode == MF_CALL_LATCH ){
		pad->Buttons |= entry->current.temp.buttons;
		return;
	}
	
	for( ; ; entry->current.command = entry->current.command->next ){
		/* 現在のコマンドがNULLの場合は、実行待ち */
		if( ! entry->current.command ){
			if( (entry->current.runLoop)-- ){
				mfRapidfireClearAll( entry->current.rfUid );
				entry->current.command = entry->current.macro;
			} else{
				macro_stop( entry );
				break;
			}
		}
		
		/* コマンドごとの処理 */
		if( entry->current.command->action == MA_DELAY ){
			uint64_t rtc;
			sceRtcGetCurrentTick( &rtc );
			
			if( ( rtc - entry->current.temp.rtc ) / 1000 >= entry->current.command->data ){
				entry->current.temp.rtc = 0;
				continue;
			}
		} else if( entry->current.command->action == MA_BUTTONS_PRESS ){
			if( ! entry->current.command->data ) continue;
			entry->current.temp.buttons |= entry->current.command->data;
		} else if( entry->current.command->action == MA_BUTTONS_RELEASE ){
			if( ! entry->current.command->data ) continue;
			entry->current.temp.buttons ^= entry->current.command->data;
		} else if( entry->current.command->action == MA_BUTTONS_CHANGE ){
			if( ! entry->current.command->data ) continue;
			entry->current.temp.buttons = entry->current.command->data;
		} else if( entry->current.command->action == MA_ANALOG_MOVE ){
			entry->current.temp.analogCoord = entry->current.command->data;
		} else if( entry->current.command->action == MA_RAPIDFIRE_START ){
			if( ! entry->current.command->data ) continue;
			mfRapidfireSet(
				entry->current.rfUid,
				(unsigned int)(entry->current.command->data),
				MF_RAPIDFIRE_MODE_AUTORAPID,
				MACROMGR_GET_RAPIDPDELAY( entry->current.command->sub ),
				MACROMGR_GET_RAPIDRDELAY( entry->current.command->sub )
			);
		} else if( entry->current.command->action == MA_RAPIDFIRE_STOP ){
			if( ! entry->current.command->data ) continue;
			mfRapidfireClear( entry->current.rfUid, (unsigned int)(entry->current.command->data) );
		}
		
		mfRapidfire( entry->current.rfUid, mode, pad );
		
		pad->Buttons |= entry->current.temp.buttons;
		
		coord = MACROMGR_GET_ANALOG_X( entry->current.temp.analogCoord );
		if( abs( coord - MACROMGR_ANALOG_CENTER ) > abs( pad->Lx - MACROMGR_ANALOG_CENTER ) ) pad->Lx = coord;
		
		coord = MACROMGR_GET_ANALOG_Y( entry->current.temp.analogCoord );
		if( abs( coord - MACROMGR_ANALOG_CENTER ) > abs( pad->Ly - MACROMGR_ANALOG_CENTER ) ) pad->Ly = coord;
		
		/* 次のコマンドへ */
		if( entry->current.command->action != MA_DELAY ){
			entry->current.command = entry->current.command->next;
			sceRtcGetCurrentTick( &(entry->current.temp.rtc) );
		}
		
		/* コマンドを一つ実行完了により抜ける */
		break;
	}
}

/*-------------------------------------
	セーブ/ロード
--------------------------------------*/
static void macro_load( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen )
{
	char *macro[MACRO_INILINE_ALL_COLUMNS], *key = NULL, *value = NULL;
	char *saveptr;
	int version, prev_seq = -1, cur_seq;
	uint64_t data, *storage;
	
	if( ! inimgrGetInt( params->uid, "default", MACRO_DATA_SIGNATURE, (long *)&version ) || version <= MACRO_DATA_REFUSE_VERSION ) return;
	
	if( st_macro[st_slot].current.macro ) macro_purge( &(st_macro[st_slot]) );
	
	while( inimgrCbReadln( params, buf, buflen ) ){
		inimgrParseEntry( buf, &key, &value );
		
		macro[MACRO_INILINE_SEQ]    = strtok_r( buf,  ".", &saveptr );
		macro[MACRO_INILINE_ACTION] = strtok_r( NULL, ".", &saveptr );
		macro[MACRO_INILINE_TARGET] = strtok_r( NULL, ".", &saveptr );
		macro[MACRO_INILINE_ORDER]  = strtok_r( NULL, ".", &saveptr );
		macro[MACRO_INILINE_TYPE]   = strtok_r( NULL, ".", &saveptr );
		
		if( ! macro[MACRO_INILINE_TYPE] ) return;
		
		cur_seq = strtol( macro[MACRO_INILINE_SEQ], NULL, 10 );
		
		/* マクロシーケンスが増加した場合のみアクションを判定し、MacroDataを初期化 */
		if( cur_seq > prev_seq ){
			if( ! st_macro[st_slot].current.macro ){
				macro_create( &(st_macro[st_slot]) );
				st_macro[st_slot].current.command = st_macro[st_slot].current.macro;
			} else{
				st_macro[st_slot].current.command = macromgrInsertAfter( st_macro[st_slot].current.command );
				if( ! st_macro[st_slot].current.command ) return;
			}
			
			prev_seq = cur_seq;
			
			st_macro[st_slot].current.command->data = 0;
			st_macro[st_slot].current.command->sub  = 0;
			
			/* 初期化 */
			if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_DELAY ) == 0 ){
				st_macro[st_slot].current.command->action = MA_DELAY;
			} else if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_BUTTONS_PRESS ) == 0 ){
				st_macro[st_slot].current.command->action = MA_BUTTONS_PRESS;
			} else if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_BUTTONS_RELEASE ) == 0 ){
				st_macro[st_slot].current.command->action = MA_BUTTONS_RELEASE;
			} else if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_BUTTONS_CHANGE ) == 0 ){
				st_macro[st_slot].current.command->action = MA_BUTTONS_CHANGE;
			} else if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_ANALOG_MOVE ) == 0 ){
				st_macro[st_slot].current.command->action = MA_ANALOG_MOVE;
				st_macro[st_slot].current.command->data   = MACROMGR_ANALOG_NEUTRAL;
			} else if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_RAPIDFIRE_START ) == 0 ){
				st_macro[st_slot].current.command->action = MA_RAPIDFIRE_START;
				st_macro[st_slot].current.command->sub    = MACROMGR_SET_RAPIDDELAY( MACROMGR_DEFAULT_PRESS_DELAY, MACROMGR_DEFAULT_RELEASE_DELAY );
			} else if( strcasecmp( macro[MACRO_INILINE_ACTION], MACRO_ACTION_RAPIDFIRE_STOP ) == 0 ){
				st_macro[st_slot].current.command->action = MA_RAPIDFIRE_STOP;
			}
		} else if( cur_seq < prev_seq ){
			return;
		}
		
		/* データを作成 */
		if( strcasecmp( macro[MACRO_INILINE_TYPE], "Num" ) == 0 ){
			data = strtoul( value, NULL, 10 );
		} else if( strcasecmp( macro[MACRO_INILINE_TYPE], "Buttons" ) == 0 ){
			data = ctrlpadUtilStringToButtons( value );
		} else{
			return;
		}
		
		/* データの格納位置を確定 */
		if( strcasecmp( macro[MACRO_INILINE_TARGET], "Data" ) == 0 ){
			storage = &(st_macro[st_slot].current.command->data);
		} else if( strcasecmp( macro[MACRO_INILINE_TARGET], "Sub" ) == 0 ){
			storage = &(st_macro[st_slot].current.command->sub);
		} else{
			return;
		}
		
		/* データを格納 */
		if( strcasecmp( macro[MACRO_INILINE_ORDER], "Full" ) == 0 ){
			*storage = data;
		} else if( strcasecmp( macro[MACRO_INILINE_ORDER], "Hi" ) == 0 ){
			*storage = MACROMGR_SET_HILO_DWORD( data, MACROMGR_GET_LO_DWORD( *storage ) );
		} else if( strcasecmp( macro[MACRO_INILINE_ORDER], "Lo" ) == 0 ){
			*storage = MACROMGR_SET_HILO_DWORD( MACROMGR_GET_HI_DWORD( *storage ), data );
		} else{
			return;
		}
	}
}

static void macro_save( InimgrCallbackMode mode, InimgrCallbackParams *params, char *buf, size_t buflen )
{
	char buttons[128];
	unsigned int seq = 0, len = 0;
	
	for( st_macro[st_slot].current.command = st_macro[st_slot].current.macro; st_macro[st_slot].current.command; st_macro[st_slot].current.command = st_macro[st_slot].current.command->next ){
		switch( st_macro[st_slot].current.command->action ){
			case MA_DELAY:
				len = snprintf( buf, buflen, "%d.%s.Data.Full.Num = %llu", seq, MACRO_ACTION_DELAY, (uint64_t)st_macro[st_slot].current.command->data );
				inimgrCbWriteln( params, buf, len );
				break;
			case MA_BUTTONS_PRESS:
				ctrlpadUtilButtonsToString( st_macro[st_slot].current.command->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%d.%s.Data.Full.Buttons = %s", seq, MACRO_ACTION_BUTTONS_PRESS, buttons );
				inimgrCbWriteln( params, buf, len );
				break;
			case MA_BUTTONS_RELEASE:
				ctrlpadUtilButtonsToString( st_macro[st_slot].current.command->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%d.%s.Data.Full.Buttons = %s", seq, MACRO_ACTION_BUTTONS_RELEASE, buttons );
				inimgrCbWriteln( params, buf, len );
				break;
			case MA_BUTTONS_CHANGE:
				ctrlpadUtilButtonsToString( st_macro[st_slot].current.command->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%d.%s.Data.Full.Buttons = %s", seq, MACRO_ACTION_BUTTONS_CHANGE, buttons );
				inimgrCbWriteln( params, buf, len );
				break;
			case MA_ANALOG_MOVE:
				len = snprintf( buf, buflen, "%d.%s.Data.Hi.Num = %d", seq, MACRO_ACTION_ANALOG_MOVE, (int)MACROMGR_GET_ANALOG_X( st_macro[st_slot].current.command->data ) );
				inimgrCbWriteln( params, buf, len );
				len = snprintf( buf, buflen, "%d.%s.Data.Lo.Num = %d", seq, MACRO_ACTION_ANALOG_MOVE, (int)MACROMGR_GET_ANALOG_Y( st_macro[st_slot].current.command->data ) );
				inimgrCbWriteln( params, buf, len );
				break;
			case MA_RAPIDFIRE_START:
				ctrlpadUtilButtonsToString( st_macro[st_slot].current.command->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%d.%s.Data.Full.Buttons = %s", seq, MACRO_ACTION_RAPIDFIRE_START, buttons );
				inimgrCbWriteln( params, buf, len );
				len = snprintf( buf, buflen, "%d.%s.Sub.Hi.Num = %d", seq, MACRO_ACTION_RAPIDFIRE_START, MACROMGR_GET_RAPIDPDELAY( st_macro[st_slot].current.command->sub ) );
				inimgrCbWriteln( params, buf, len );
				len = snprintf( buf, buflen, "%d.%s.Sub.Lo.Num = %d", seq, MACRO_ACTION_RAPIDFIRE_START, MACROMGR_GET_RAPIDRDELAY( st_macro[st_slot].current.command->sub ) );
				inimgrCbWriteln( params, buf, len );
				break;
			case MA_RAPIDFIRE_STOP:
				ctrlpadUtilButtonsToString( st_macro[st_slot].current.command->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%d.%s.Data.Full.Buttons = %s", seq, MACRO_ACTION_RAPIDFIRE_STOP, buttons );
				inimgrCbWriteln( params, buf, len );
				break;
		}
		seq++;
	}
}


/*-------------------------------------
	エラーチェック
--------------------------------------*/
static bool macro_is_busy( MacroEntry *entry )
{
	return entry->current.runMode != MRM_NONE ? true : false;
}

static bool macro_is_unavail( MacroEntry *entry )
{
	return ! entry->current.macro ? true : false;
}

static bool macro_is_disable_engine( void )
{
	return mfIsDisabled() ? true : false;
}

static bool macro_is_locked( void )
{
	return st_exlock;
}

static bool macro_is_busy_with_message( MacroEntry *entry )
{
	bool rc = macro_is_busy( entry );
	
	if( rc ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Macro function is busy." );
		mfMenuWait( 1000000 );
	}
	
	return rc;
}

static bool macro_is_unavail_with_message( MacroEntry *entry )
{
	bool rc = macro_is_unavail( entry );
	
	if( rc ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "No current macro." );
		mfMenuWait( 1000000 );
	}
	
	return rc;
}

static bool macro_is_disable_engine_with_message( void )
{
	bool rc = macro_is_disable_engine();
	
	if( rc ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "MacroFire Engine is currently off." );
		mfMenuWait( 1000000 );
	}
	
	return rc;
}

static bool macro_is_locked_with_message( void )
{
	bool rc = macro_is_locked();
	
	if( rc ){
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 4 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Already running or recording in other macro slot." );
		mfMenuWait( 1000000 );
	}
	
	return rc;
}

