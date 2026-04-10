/*
	menu.c
*/

#include "menu.h"
#include "mftable.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void mf_indicator( void );
static void mf_threads_stat_change( enum mf_thread_chstat stat, SceUID thlist[], int thnum );
static void mf_emerg_menu( void );
static void mf_menu_create_home_menu( MfMenuItem *menu, int menu_num, MfMenuCallback *mf_menu );
static void mf_backup_fbstat( struct mf_fbstat *fbstat );
static void mf_restore_fbstat( struct mf_fbstat *fbstat );
static bool mf_alloc_buffers( struct mf_buffers *buf, void *orig_buffer );
static void mf_free_buffers( struct mf_buffers *buf );
static void mf_ready_to_draw( void );
static void mf_draw_base( void );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static bool          st_interrupt = true;
static bool          st_menu_quit = false;
static bool          st_dialog    = false;
static uint64_t      st_wait_micro_sec = 0;
static SceUID        st_thids_first[MFM_MAX_NUMBER_OF_THREADS];
static int           st_thnum_first;
static SceCtrlData   st_pad_data;
static CtrlpadParams st_cp_params;
static void          *st_vrambase;

static char          st_label[MFM_ITEM_LENGTH];
static char          st_usage[MFM_ITEM_LENGTH];

/*=============================================*/

static void mf_indicator( void )
{
	static char st_working_indicator[] = { '|', '/', '-', '\\' };
	static int  st_working_indicator_index = 0;
	
	if( st_working_indicator_index > ( sizeof( st_working_indicator ) - 1 ) ) st_working_indicator_index = 0;
	
	gbPutChar( gbOffsetChar( 1 ), gbOffsetLine( 1 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT, st_working_indicator[st_working_indicator_index++] );
}

static void mf_threads_stat_change( enum mf_thread_chstat stat, SceUID thids[], int thnum )
{
	int ( *request_stat_func )( SceUID ) = NULL;
	int i, j;
	SceUID selfid = sceKernelGetThreadId();
	
	switch( stat ){
		case MFM_THREADS_SUSPEND:
			request_stat_func = sceKernelSuspendThread;
			break;
		case MFM_THREADS_RESUME:
			request_stat_func = sceKernelResumeThread;
			break;
	}
	
	for( i = 0; i < thnum; i++ ){
		bool not_target = false;
		for( j = 0; j < st_thnum_first; j++ ){
			if( thids[i] == st_thids_first[j] || selfid == thids[i] ){
				not_target = true;
				break;
			}
		}
		if( ! not_target ) ( *request_stat_func )( thids[i] );
	}
}

bool mfMenuInit( void )
{
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, st_thids_first, MFM_MAX_NUMBER_OF_THREADS, &st_thnum_first );
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	return true;
}
/*
void mfDebug( struct mf_buffers *buf )
{
	static int fps = 0;
	static int st_fps = 0;
	static uint64_t st_fps_tick_now = 0;
	static uint64_t st_fps_tick_last = 0;
	
	st_fps++;
	sceRtcGetCurrentTick( &st_fps_tick_now );
	if( (st_fps_tick_now - st_fps_tick_last) >= 1000000 ){
		st_fps_tick_last = st_fps_tick_now;
		fps = st_fps;
		st_fps = 0;
	}

	
	gbPrintf( gbOffsetChar( 1 ), gbOffsetLine( 0 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT,
		"%d",
		mfRunEnv()
	);
		
	gbPrintf( gbOffsetChar( 1 ), gbOffsetLine( 2 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT,
		"%d 1:%d 2:%d 3:%d 4:%d 5:%d 6:%d",
		fps,
		sceKernelPartitionTotalFreeMemSize( 1 ),
		sceKernelPartitionTotalFreeMemSize( 2 ),
		sceKernelPartitionTotalFreeMemSize( 3 ),
		sceKernelPartitionTotalFreeMemSize( 4 ),
		sceKernelPartitionTotalFreeMemSize( 5 ),
		sceKernelPartitionTotalFreeMemSize( 6 )
	);
}
*/
static void mf_backup_fbstat( struct mf_fbstat *fbstat )
{
	fbstat->mode        = gbGetMode();
	fbstat->width       = gbGetWidth();
	fbstat->height      = gbGetHeight();
	fbstat->frameBuffer = gbGetCurrentDisplayBuf();
	fbstat->bufferWidth = gbGetBufferWidth();
	fbstat->pixelFormat = gbGetPixelFormat();
}

static void mf_restore_fbstat( struct mf_fbstat *fbstat )
{
	sceDisplaySetMode( fbstat->mode, fbstat->width, fbstat->height );
	sceDisplaySetFrameBuf( fbstat->frameBuffer, fbstat->bufferWidth, fbstat->pixelFormat, PSP_DISPLAY_SETBUF_IMMEDIATE );
}

static bool mf_alloc_buffers( struct mf_buffers *buf, void *orig_buffer )
{
	int backup_vram_size;
	
	memset( buf, 0, sizeof( struct mf_buffers ) );
	
	buf->frame.size = gbGetDataFrameSize();
	backup_vram_size = buf->frame.size * 3;
	
	if( scePowerVolatileMemTryLock( 0, &(buf->backup.vram), &backup_vram_size ) == 0 ){
		sceDmacMemcpy( buf->backup.vram, st_vrambase, backup_vram_size );
		buf->frame.display    = st_vrambase;
		buf->frame.draw       = (void *)( (unsigned int)(buf->frame.display) + buf->frame.size );
		buf->frame.clearImage = (void *)( (unsigned int)(buf->frame.draw) + buf->frame.size );
		buf->useVolatileMem   = true;
	} else{
		unsigned int frame_num = sceKernelMaxFreeMemSize() / buf->frame.size;
		
		if( frame_num >= 3 ){
			buf->backup.vram = memsceMalloc( backup_vram_size );
			if( ! buf->backup.vram ) return false;
			
			sceDmacMemcpy( buf->backup.vram, st_vrambase, backup_vram_size );
			buf->frame.display    = st_vrambase;
			buf->frame.draw       = (void *)( (unsigned int)(buf->frame.display) + buf->frame.size );
			buf->frame.clearImage = (void *)( (unsigned int)(buf->frame.draw) + buf->frame.size );
		} else if( frame_num == 2 ){
			buf->frame.display = orig_buffer;
			buf->frame.draw    = memsceMalloc( buf->frame.size * 2 );
			if( ! buf->frame.draw ) return false;
			buf->frame.clearImage = (void *)( (unsigned int)(buf->frame.draw) + buf->frame.size );
		} else if( frame_num == 1 ){
			buf->frame.display = orig_buffer;
			buf->frame.draw    = memsceMalloc( buf->frame.size );
			if( ! buf->frame.draw ) return false;
		} else{
			return false;
		}
	}
	return true;
};

static void mf_free_buffers( struct mf_buffers *buf )
{
	if( buf->backup.vram ){
		sceDmacMemcpy( st_vrambase, buf->backup.vram, buf->frame.size * 3 );
	}
	
	if( buf->useVolatileMem ){
		scePowerVolatileMemUnlock( 0 );
	} else if( buf->backup.vram ){
		memsceFree( buf->backup.vram );
	} else if( buf->frame.draw ){
		memsceFree( buf->frame.draw );
	}
}

static void mf_ready_to_draw( void )
{
	gbFillRect( 0, 0, SCR_WIDTH, SCR_HEIGHT, MFM_BG_COLOR );
}

static void mf_draw_base( void )
{
	gbFillRect( 0, 0, SCR_WIDTH, gbOffsetLine( 3 ), MFM_TITLE_BAR_COLOR );
	gbFillRect( 0, SCR_HEIGHT - gbOffsetLine( 3 ), SCR_WIDTH, SCR_HEIGHT, MFM_INFO_BAR_COLOR );
	gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 1 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT, MFM_TOP_MESSAGE );
}

void mfMenu( void )
{
	SceUID thlist[MFM_MAX_NUMBER_OF_THREADS];
	int thnum;
	
	MfMenuRc       rc = MR_CONTINUE;
	MfMenuItem     *menu = NULL;
	MfMenuCallback mf_menu;
	int            menu_num, menu_selected;
	
	bool mf_api_hooked = mfIsApiHooked();
	struct mf_fbstat  fbstat;
	struct mf_buffers buf;
	
	/* 他のスレッドをサスペンド */
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MFM_MAX_NUMBER_OF_THREADS, &thnum );
	mf_threads_stat_change( MFM_THREADS_SUSPEND, thlist, thnum );
	
	/* APIがフックされていれば一時的に解除 */
	if( mf_api_hooked ) mfRestoreApi();
	
	/* VRAMの先頭アドレスを取得 */
	st_vrambase = (void *)( (unsigned int)sceGeEdramGetAddr() | GB_NOCACHE );
	
	/* gbライブラリの初期化と設定 */
	if( gbBind() < 0 ) goto QUIT;
	gbSetOpt( GB_AUTOFLUSH | GB_ALPHABLEND );
	gbUse8BitFont( GB_FONT_PSPSYM );
	gbPrepare();
	
	/* 現在のフレームバッファ情報を保存 */
	mf_backup_fbstat( &fbstat );
	
	/* バッファを確保 */
	if( ! mf_alloc_buffers( &buf, fbstat.frameBuffer ) ){
		mf_emerg_menu();
		goto QUIT;
	}
	
	/* クリア用背景をVRAMの3枚目に保存 */
	if( buf.frame.clearImage ){
		/* 描画準備 */
		gbSetDisplayBuf( buf.frame.clearImage );
		gbSetDrawBuf( NULL );
		gbPrepare();
		
		/* 現在表示されている画面をクリア用背景にコピー */
		sceDisplayWaitVblankStart();
		sceDmacMemcpy( buf.frame.clearImage, fbstat.frameBuffer, buf.frame.size );
		
		/* クリア用背景へ基本となるグラフィックを描画 */
		mf_ready_to_draw();
		mf_draw_base();
	}
	
	/* VRAMの1枚目と2枚目を表示バッファと描画バッファに使う */
	gbSetDisplayBuf( buf.frame.display );
	gbSetDrawBuf( buf.frame.draw );
	
	/* 設定したフレームバッファを適用 */
	gbPrepare();
	
	/* 基本メニューを作成する */
	menu_num = mftableEntry + MFM_MAIN_MENU_COUNT;
	menu     = (MfMenuItem *)memsceMalloc( sizeof( MfMenuItem ) * menu_num );
	if( ! menu ){
		mf_emerg_menu();
		goto DESTROY;
	}
	mf_menu_create_home_menu( menu, menu_num, &mf_menu );
	
	/* パッドのキーリピート計算機を初期化 */
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	
	mfMenuEnableInterrupt();
	sceDisplayWaitVblankStart();
	
	while( gRunning ){
		/* 背景を描画 */
		if( buf.frame.clearImage ){
			sceDmacMemcpy( gbGetCurrentDrawBuf(), buf.frame.clearImage, buf.frame.size );
		} else{
			memset( gbGetCurrentDrawBuf(), 0, buf.frame.size );
			gbRmvOpt( GB_ALPHABLEND );
			mf_draw_base();
			gbAddOpt( GB_ALPHABLEND );
		}
		
		/* インジケータを描画 */
		mf_indicator();
		
		st_pad_data.Buttons = ctrlpadGetData( &st_cp_params, &st_pad_data, CTRLPAD_IGNORE_ANALOG_DIRECTION );
		
		if( ( st_interrupt && st_pad_data.Buttons & ( PSP_CTRL_START | PSP_CTRL_HOME ) ) || st_menu_quit ){
			if( rc == MR_ENTER && (MfFuncTerm)MFM_GET_CB_ARG_BY_PTR( mf_menu.arg, 0 ) ) ( (MfFuncTerm)MFM_GET_CB_ARG_BY_PTR( mf_menu.arg, 0 ) )();
			st_menu_quit = false;
			break;
		}
		
		//mfDebug( &buf );
		
		if( rc == MR_BACK ){
			mfMenuQuit();
		} else if( rc == MR_CONTINUE ){
			rc = mfMenuUniDraw( gbOffsetChar( 5 ), gbOffsetLine( 4 ), menu, menu_num, &menu_selected, 0 );
		} else{
			MfMenuRc func_rc = ( (MfFuncMenu)mf_menu.func )( &st_pad_data, NULL );
			if( func_rc == MR_BACK ){
				if( (MfFuncTerm)MFM_GET_CB_ARG_BY_PTR( mf_menu.arg, 0 ) ) ( (MfFuncTerm)MFM_GET_CB_ARG_BY_PTR( mf_menu.arg, 0 ) )();
				rc = MR_CONTINUE;
			}
		}
		
		/* 表示バッファと描画バッファを切り替える */
		gbSwapBuffers();
		sceDisplayWaitVblankStart();
		
		if( st_wait_micro_sec ){
			sceKernelDelayThread( st_wait_micro_sec );
			st_wait_micro_sec = 0;
		}
	}
	
	goto DESTROY;
	
	DESTROY:
		/* メニューリストを解放 */
		if( menu ) memsceFree( menu );
		
		gbPrint( gbOffsetChar( 30 ), gbOffsetLine( 17 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Returning to the game..." );
		gbSwapBuffers();
		sceKernelDelayThread( 300000 );
		
		sceDisplayWaitVblankStart();
		
		/* 表示バッファを元の位置に戻す */
		mf_restore_fbstat( &fbstat );
		
		/* バッファを解放 */
		mf_free_buffers( &buf );
		
		goto QUIT;
	
	QUIT:
		/* APIがフックされていたのならば再度フック */
		if( mf_api_hooked ) mfHookApi();
		
		sceDisplayWaitVblankStart();
		
		/* 他のスレッドをリジューム */
		mf_threads_stat_change( MFM_THREADS_RESUME, thlist, thnum );
		
		/* LCDの点灯を待つ */
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		sceDisplayEnable();
}

void mfMenuDisableInterrupt( void )
{
	st_interrupt = false;
}

void mfMenuEnableInterrupt( void )
{
	st_interrupt = true;
}

void mfMenuWait( uint64_t micro_sec )
{
	st_wait_micro_sec = micro_sec;
}

void mfMenuQuit( void )
{
	st_menu_quit = true;
}

void mfMenuKeyRepeatReset( void )
{
	ctrlpadUpdateData( &st_cp_params );
}

void mfMenuUsage( const char *format, ... )
{
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( st_usage, MFM_ITEM_LENGTH, format, ap );
	va_end( ap );
}

void mfMenuLabel( const char *format, ... )
{
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( st_label, MFM_ITEM_LENGTH, format, ap );
	va_end( ap );
}

MfMenuRc mfMenuUniDraw( int x, int y, MfMenuItem menu[], size_t max, int *select, MfMenuOptions options )
{
	int current;
	
	if( *select < 0 ){
		*select = 0;
	} else if( *select >= max ){
		*select = max - 1;
	}
	
	for( current = 0; current < max; current++ ){
		if( menu[current].label == NULL ) continue;
		
		/* メニュー項目文字列作成 */
		( menu[current].proc )( MS_QUERY_LABEL, &st_pad_data, menu[current].var, menu[current].value, (void *)(menu[current].label) );
		
		/* メニュー項目 */
		gbPrint( x, y + gbOffsetLine( current ), current == *select ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, st_label );
	}
	
	/* 操作方法文字列作成 */
	( menu[*select].proc )( MS_QUERY_USAGE, &st_pad_data, menu[*select].var, menu[*select].value, NULL );
	
	if( options & MO_DISPLAY_ONLY ) return MR_CONTINUE;
	
	if( ! st_dialog ){
		/* 操作方法 */
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 32 ) - ( GB_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80\x82 = Move, \x86 = Back, START/HOME = Exit" );
		gbPrint( gbOffsetChar( 3 ), gbOffsetLine( 32 ) + ( GB_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, st_usage );
		
		if( st_pad_data.Buttons & PSP_CTRL_UP ){
			do{
				(*select)--;
				if( *select < 0 ) *select = max - 1;
			} while( menu[*select].label == NULL );
		} else if( st_pad_data.Buttons & PSP_CTRL_DOWN ){
			do{
				(*select)++;
				if( *select >= max ) *select = 0;
			} while( menu[*select].label == NULL );
		} else if( st_pad_data.Buttons & PSP_CTRL_CROSS ){
			return MR_BACK;
		}
		
		return ( menu[*select].proc )( MS_NONE, &st_pad_data, menu[*select].var, menu[*select].value, NULL );
	} else{
		if( mfMenuGetNumberIsReady() ){
			MfMenuRc rc = mfMenuGetNumber();
			if( rc != MR_CONTINUE ) st_dialog = false;
		} else if( mfMenuGetButtonsIsReady() ){
			MfMenuRc rc = mfMenuGetButtons();
			if( rc != MR_CONTINUE ) st_dialog = false;
		}
		
		return MR_CONTINUE;
	}
}

static void mf_emerg_menu( void )
{
	mf_ready_to_draw();
	mf_draw_base();
	
	gbPrint( gbOffsetChar( 5 ), gbOffsetLine(  4 ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, "Failed to allocate memory for operation." );
	gbPrint( gbOffsetChar( 5 ), gbOffsetLine(  6 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "There is not enough available memory." );
	gbPrint( gbOffsetChar( 5 ), gbOffsetLine(  7 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "So, MacroFire is launching the emergency dialog." );
	
	gbPrint( gbOffsetChar( 5 ), gbOffsetLine( 9 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Waiting for button press..." );
	gbPrint( gbOffsetChar( 9 ), gbOffsetLine( 10 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "START/HOME = Quit." );
	gbPrint( gbOffsetChar( 9 ), gbOffsetLine( 11 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "SELECT     = Toggling MacroFire Engine state and quit." );
	
	gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 13 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Currently MacroFire Engine: %s", gMfEngine ? "ON" : "OFF" );
	
	do{
		sceCtrlPeekBufferPositive( &st_pad_data, 1 );
		sceKernelDelayThread( 10000 );
	} while( ! ( st_pad_data.Buttons & ( PSP_CTRL_START | PSP_CTRL_SELECT | PSP_CTRL_HOME ) ) );
	
	if( st_pad_data.Buttons & ( PSP_CTRL_START | PSP_CTRL_HOME ) ){
		gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 16 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Quit." );
	} else if( st_pad_data.Buttons & PSP_CTRL_SELECT ){
		if( mfIsEnabled() ){
			mfDisable();
		} else{
			mfEnable();
		}
		gbPrintf( gbOffsetChar( 5 ), gbOffsetLine( 16 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "MacroFire Engine is toggled.\nQuit." );
	}
	
	sceKernelDelayThread( 2000000 );
}

static void mf_menu_create_home_menu( MfMenuItem *menu, int menu_num, MfMenuCallback *mf_menu )
{
	int mftable_index;
	
	menu[0].label           = "MacroFire Engine: %s";
	menu[0].proc            = mfMenuDefBoolProc;
	menu[0].var             = &gMfEngine;
	menu[0].value[0].string = "OFF";
	menu[0].value[1].string = "ON";
	
	menu[1].label            = NULL;
	
	menu[2].label            = "Buttons to launch the menu";
	menu[2].proc             = mfMenuDefGetButtonsProc;
	menu[2].var              = &gMfMenu;
	menu[2].value[0].string  = "Set to launch buttons";
	menu[2].value[1].integer = MFM_ALL_AVAILABLE_BUTTONS;
	
	menu[3].label            = "Buttons to toggle the engine state";
	menu[3].proc             = mfMenuDefGetButtonsProc;
	menu[3].var              = &gMfToggle;
	menu[3].value[0].string  = "Set to toggle buttons";
	menu[3].value[1].integer = MFM_ALL_AVAILABLE_BUTTONS;
	
	menu[4].label            = "Tuning the analog stick sensitivity";
	menu[4].proc             = mfMenuDefCallbackProc;
	menu[4].var              = mf_menu;
	menu[4].value[0].pointer = analogtuneMenu;
	menu[4].value[1].pointer = NULL;
	
	menu[5].label            = NULL;
	
	for( mftable_index = MFM_MAIN_MENU_COUNT; mftable_index < menu_num; mftable_index++ ){
		menu[mftable_index].label            = mftable[mftable_index - MFM_MAIN_MENU_COUNT].name;
		menu[mftable_index].proc             = mfMenuDefCallbackProc;
		menu[mftable_index].var              = mf_menu;
		menu[mftable_index].value[0].pointer = mftable[mftable_index - MFM_MAIN_MENU_COUNT].menu.func;
		menu[mftable_index].value[1].pointer = mftable[mftable_index - MFM_MAIN_MENU_COUNT].menu.quit;
	}
}

static void mfRunDialog( void )
{
	st_dialog = true;
}

/*-------------------------------------
	デフォルトプロシージャ
-------------------------------------*/
MfMenuRc mfMenuDefAnchorProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	return MR_CONTINUE;;
}

MfMenuRc mfMenuDefCallbackProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	if( signal == MS_QUERY_USAGE ){
		mfMenuUsage( "\x85 = Enter" );
	} else if( signal == MS_QUERY_LABEL ){
		mfMenuLabel( (char *)arg );
	} else{
		if( pad->Buttons & PSP_CTRL_CIRCLE ){
			((MfMenuCallback *)(var))->func = value[0].pointer;
			((MfMenuCallback *)(var))->arg  = &(value[1]);
			return MR_ENTER;
		}
	}
	return MR_CONTINUE;
}

MfMenuRc mfMenuDefBoolProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	if( signal == MS_QUERY_USAGE ){
		mfMenuUsage( "\x83\x81 = Toggle switch" );
	} else if( signal == MS_QUERY_LABEL ){
		mfMenuLabel( (char *)arg, *((bool *)var) ? value[1].string :value[0].string );
	} else{
		if( pad->Buttons & ( PSP_CTRL_LEFT | PSP_CTRL_RIGHT ) ){
			*((bool *)var) = *((bool *)var) ? false : true;
		}
	}
	return MR_CONTINUE;
}

MfMenuRc mfMenuDefOptionProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	if( signal == MS_QUERY_USAGE ){
		mfMenuUsage( "\x83\x81 = Change option" );
	} else if( signal == MS_QUERY_LABEL ){
		mfMenuLabel( (char *)arg, ((char **)value[0].pointer)[*((int *)var)] );
	} else{
		if( pad->Buttons & PSP_CTRL_LEFT ){
			if( *((int *)var) == 0 ){
				*((int *)var) = value[1].integer - 1;
			} else{
				(*((int *)var))--;
			}
		} else if( pad->Buttons & PSP_CTRL_RIGHT ){
			if( *((int *)var) + 1 >= value[1].integer ){
				*((int *)var) = 0;
			} else{
				(*((int *)var))++;
			}
		}
	}
	return MR_CONTINUE;
}

MfMenuRc mfMenuDefGetNumberProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	if( signal == MS_QUERY_USAGE ){
		mfMenuUsage( "\x83 = Decrease, \x81 = Increase, \x85 = Edit" );
	} else if( signal == MS_QUERY_LABEL ){
		mfMenuLabel( (char *)arg, *((int *)var), value[1].string );
	} else{
		if( pad->Buttons & PSP_CTRL_LEFT ){
			if( *((int *)var) > value[3].integer ){
				(*((int *)var)) -= value[3].integer;
			} else{
				(*((int *)var)) = 0;
			}
		} else if( pad->Buttons & PSP_CTRL_RIGHT ){
			(*((int *)var)) += value[3].integer;
			if( *((int *)var) > value[4].integer ){
				*((int *)var) = value[4].integer;
			}
		} else if( pad->Buttons & PSP_CTRL_CIRCLE ){
			if( 
				mfMenuGetNumberInit(
					value[0].string,
					value[1].string,
					(long *)(var),
					value[2].integer
				) == MR_ENTER
			){
				mfRunDialog();
			}
		}
	}
	return MR_CONTINUE;
}

MfMenuRc mfMenuDefGetButtonsProc( MfMenuCtrlSignal signal, SceCtrlData *pad, void *var, MfMenuItemValue value[], const void *arg )
{
	if( signal == MS_QUERY_USAGE ){
		mfMenuUsage( "\x85 = Edit" );
	} else if( signal == MS_QUERY_LABEL ){
		mfMenuLabel( (char *)arg );
	} else{
		if( pad->Buttons & PSP_CTRL_CIRCLE ){
			if(
				mfMenuGetButtonsInit(
					value[0].string,
					(unsigned int *)(var),
					(unsigned int)(value[1].integer)
				) == MR_ENTER
			){
				mfRunDialog();
			}
		}
	}
	return MR_CONTINUE;
}


/*-------------------------------------
	ダイアログラッパ
-------------------------------------*/
bool mfMenuGetNumberIsReady( void )
{
	if( cmndlgGetNumbersGetStatus() != CMNDLG_NONE ){
		return true;
	} else{
		return false;
	}
}

MfMenuRc mfMenuGetNumberInit( const char *title, const char *unit, long *number, int digits )
{
	CmndlgGetNumbersParams *params;
	
	mfMenuDisableInterrupt();
	
	params = (CmndlgGetNumbersParams *)memsceMalloc( sizeof( CmndlgGetNumbersParams ) );
	if( ! params ){
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	params->data = (CmndlgGetNumbersData *)memsceMalloc( sizeof( CmndlgGetNumbersData ) );
	if( ! params->data ){
		memsceFree( params );
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	params->numberOfData        = 1;
	params->selectDataNumber    = 0;
	params->rc                  = 0;
	params->ui.x                = gbOffsetChar( 39 );
	params->ui.y                = gbOffsetLine( 6 );
	params->ui.fgTextColor      = MFM_TEXT_FGCOLOR;
	params->ui.fcTextColor      = MFM_TEXT_FCCOLOR;
	params->ui.bgTextColor      = MFM_TRANSPARENT;
	params->ui.bgColor          = MFM_BG_COLOR;
	params->ui.borderColor      = MFM_TRANSPARENT;
	params->data->title         = title;
	params->data->unit          = unit;
	params->data->numSave       = number;
	params->data->numDigits     = digits;
	params->data->selectedPlace = 0;
	
	if( cmndlgGetNumbersStart( params ) ){
		cmndlgGetNumbersShutdownStart();
		memsceFree( params->data );
		memsceFree( params );
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	return MR_ENTER;
}

MfMenuRc mfMenuGetNumber( void )
{
	CmndlgState state;
	if( cmndlgGetNumbersUpdate() ){
		//エラー
	}
	state = cmndlgGetNumbersGetStatus();
	if( state == CMNDLG_SHUTDOWN ){
		CmndlgRc rc;
		CmndlgGetNumbersParams *params = cmndlgGetNumbersGetParams();
		
		rc = params->rc;
		
		cmndlgGetNumbersShutdownStart();
		
		memsceFree( params->data );
		memsceFree( params );
		mfMenuKeyRepeatReset();
		mfMenuEnableInterrupt();
		return rc == CMNDLG_ACCEPT ? MR_ENTER : MR_BACK;
	}
	
	return MR_CONTINUE;
}

bool mfMenuGetButtonsIsReady( void )
{
	if( cmndlgGetButtonsGetStatus() != CMNDLG_NONE ){
		return true;
	} else{
		return false;
	}
}

MfMenuRc mfMenuGetButtonsInit( const char *title, unsigned int *buttons, unsigned int avail )
{
	CmndlgGetButtonsParams *params;
	
	mfMenuDisableInterrupt();
	
	params = (CmndlgGetButtonsParams *)memsceMalloc( sizeof( CmndlgGetButtonsParams ) );
	if( ! params ){
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	params->data = (CmndlgGetButtonsData *)memsceMalloc( sizeof( CmndlgGetButtonsData ) );
	if( ! params->data ){
		memsceFree( params );
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	params->numberOfData           = 1;
	params->selectDataNumber       = 0;
	params->rc                     = 0;
	params->ui.x                   = gbOffsetChar( 39 );
	params->ui.y                   = gbOffsetLine( 4 );
	params->ui.fgTextColor         = MFM_TEXT_FGCOLOR;
	params->ui.fcTextColor         = MFM_TEXT_FCCOLOR;
	params->ui.bgTextColor         = MFM_TRANSPARENT;
	params->ui.bgColor             = MFM_BG_COLOR;
	params->ui.borderColor         = MFM_TRANSPARENT;
	params->data->title            = title;
	params->data->buttonsSave      = buttons;
	params->data->buttonsAvailable = avail;
	
	if( cmndlgGetButtonsStart( params ) ){
		cmndlgGetButtonsShutdownStart();
		memsceFree( params->data );
		memsceFree( params );
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	return MR_ENTER;
}

MfMenuRc mfMenuGetButtons( void )
{
	CmndlgState state;
	if( cmndlgGetButtonsUpdate() ){
		//エラー
	}
	state = cmndlgGetButtonsGetStatus();
	if( state == CMNDLG_SHUTDOWN ){
		CmndlgRc rc;
		CmndlgGetButtonsParams *params = cmndlgGetButtonsGetParams();
		
		rc = params->rc;
		
		cmndlgGetButtonsShutdownStart();
		
		memsceFree( params->data );
		memsceFree( params );
		mfMenuKeyRepeatReset();
		mfMenuEnableInterrupt();
		return rc == CMNDLG_ACCEPT ? MR_ENTER : MR_BACK;
	}
	
	return MR_CONTINUE;
}

bool mfMenuGetFilenameIsReady( void )
{
	if( cmndlgGetFilenameGetStatus() != CMNDLG_NONE ){
		return true;
	} else{
		return false;
	}
}

MfMenuRc mfMenuGetFilenameInit( const char *title, unsigned int flags, const char *initpath )
{
	CmndlgGetFilenameParams *params;
	char *buf;
	
	mfMenuDisableInterrupt();
	
	params = (CmndlgGetFilenameParams *)memsceMalloc( sizeof( CmndlgGetFilenameParams ) );
	if( ! params ){
		//エラー
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	params->data = (CmndlgGetFilenameData *)memsceMalloc( sizeof( CmndlgGetFilenameData ) );
	if( ! params->data ){
		memsceFree( params );
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	buf = (char *)memsceMalloc( 256 );
	if( ! buf ){
		memsceFree( params->data );
		memsceFree( params );
		return MR_BACK;
	}
	
	params->numberOfData         = 1;
	params->selectDataNumber     = 0;
	params->rc                   = 0;
	params->ui.x                 = gbOffsetChar( 0 );
	params->ui.y                 = gbOffsetLine( 3 );
	params->ui.w                 = 80;
	params->ui.h                 = 28;
	params->ui.fgTextColor       = MFM_TEXT_FGCOLOR;
	params->ui.fcTextColor       = MFM_TEXT_FCCOLOR;
	params->ui.bgTextColor       = MFM_TRANSPARENT;
	params->ui.bgColor           = MFM_TRANSPARENT;
	params->ui.borderColor       = MFM_TRANSPARENT;
	params->extraUi.dirColor     = 0xffff0000;
	params->data->title          = title;
	params->data->flags          = flags;
	params->data->initPath       = initpath;
	params->data->path           = buf;
	params->data->pathMax        = 128;
	params->data->name           = buf + 128;
	params->data->nameMax        = 128;
	params->data->path[0]        = '\0';
	params->data->name[0]        = '\0';
	
	if( cmndlgGetFilenameStart( params ) ){
		cmndlgGetFilenameShutdownStart();
		memsceFree( params->data->path );
		memsceFree( params->data );
		memsceFree( params );
		mfMenuEnableInterrupt();
		return MR_BACK;
	}
	
	return MR_ENTER;
}

MfMenuRc mfMenuGetFilename( char **path, char **name )
{
	CmndlgState state;
	if( cmndlgGetFilenameUpdate() ){
		//エラー
	}
	state = cmndlgGetFilenameGetStatus();
	if( state == CMNDLG_SHUTDOWN ){
		CmndlgGetFilenameParams *params = cmndlgGetFilenameGetParams();
		
		mfMenuKeyRepeatReset();
		mfMenuEnableInterrupt();
		
		if( params->rc == CMNDLG_ACCEPT ){
			if( path ){
				*path = params->data->path;
			}
			if( name ){
				*name = params->data->name;
			}
			return MR_ENTER;
		} else{
			mfMenuGetFilenameFree();
			return MR_BACK;
		}
	}
	
	return MR_CONTINUE;
}

void mfMenuGetFilenameFree( void )
{
	CmndlgGetFilenameParams *params = cmndlgGetFilenameGetParams();
	
	cmndlgGetFilenameShutdownStart();
	
	memsceFree( params->data->path );
	memsceFree( params->data );
	memsceFree( params );
}
