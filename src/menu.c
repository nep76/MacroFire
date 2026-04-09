/*
	menu.c
*/

#include "menu.h"
#include "mftable.h"

/*-----------------------------------------------
	ローカル関数
-----------------------------------------------*/
static void mf_indicator( int *i );
static void mf_threads_stat_change( enum mf_thread_chstat stat, SceUID thlist[], int thnum );
static void mf_menu_create_item_name( char line[], size_t max, MfMenuItem *item );
static void mf_not_enough_mem_error( void );
static MfMenuRc mf_menu_control( SceCtrlData *pad_data, MfMenuItem *item );
static void mf_menu_create_base_menu( MfMenuItem *menu, int menu_num, MfMenuCallback *mf_menu );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static bool              st_interrupt = true;
static bool              st_menu_quit = false;
static bool              st_dialog    = false;
static uint64_t          st_wait_micro_sec = 0;
static SceUID            st_thids_first[MFM_MAX_NUMBER_OF_THREADS];
static int               st_thnum_first;
static SceCtrlData       st_pad_data;
static CtrlpadParams     st_cp_params;

/*=============================================*/

static void mf_indicator( int *i )
{
	char work_indicator[] = { '|', '/', '-', '\\' };
	
	if( *i > 3 ) *i = 0;
	
	blitChar( blitOffsetChar( 1 ), blitOffsetLine( 1 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT, work_indicator[*i] );
	
	(*i)++;
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

void *mfKernelVolatileMemAlloc( size_t size )
{
	if( ! size ) return NULL;
	return memsceMallocEx( 16, PSP_MEMPART_KERNEL_4, "Heap for MacroFire", PSP_SMEM_Low, size, NULL ); 
}

void mfKernelVolatileMemFree( void *addr )
{
	if( addr ) memsceFree( addr );
}

bool mfMenuInit( void )
{
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, st_thids_first, MFM_MAX_NUMBER_OF_THREADS, &st_thnum_first );
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );
	return true;
}

void mfDebug( void )
{
	static int idx = 0;
	static char work_indicator[] = { '|', '/', '-', '\\' };
	
	char cwd[255];
	
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
	
	if( idx > 3 ) idx = 0;
	
	sceIoGetThreadCwd( sceKernelGetThreadId(), cwd, sizeof( cwd ) );
	
	blitChar( blitOffsetChar( 1 ), blitOffsetLine( 1 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT, work_indicator[idx++] );
	
	blitStringf( blitOffsetChar( 1 ), blitOffsetLine( 0 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT, "%x %d %d", ini, sceKernelTotalFreeMemSize(), fps );
}

void mfMenu( void )
{
	SceUID thlist[MFM_MAX_NUMBER_OF_THREADS];
	int    thnum, indicator = 0;
	
	MfMenuRc       rc = MR_CONTINUE;
	MfMenuItem     *menu = NULL;
	MfMenuCallback mf_menu;
	int            menu_num, menu_selected;
	
	bool mf_api_hooked = mfIsApiHooked();
	bool out_of_memory = false;
	
	FbmgrDisplayStat backup_fb;
	size_t           frame_buf_size;
	void             *clear_raw_image, *return_raw_image, *internal_frame_buf;
	
	/* スレッドリストを取得 */
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MFM_MAX_NUMBER_OF_THREADS, &thnum );
	
	/* 他のスレッドをサスペンド */
	mf_threads_stat_change( MFM_THREADS_SUSPEND, thlist, thnum );
	
	/* APIがフックされていれば一時的に解除 */
	if( mf_api_hooked ) mfRestoreApi();
	
	/* Blitterを初期化 */
	blitInit();
	blit8BitCharTableSwitch( B8_BUTTON_SYMBOL );
	
	/* 現在のフレームバッファ情報を保存 */
	blitGetFrameBufStat( &backup_fb );
	
	/* フレームバッファのサイズを取得 */
	frame_buf_size = blitGetFrameBufPixelLength() * backup_fb.bufferWidth * backup_fb.height;
	
	/*
		0x88400000から開始される4MBのカーネルメモリのアクセス権を変更。
		
		sceKernelVolatileMemLock()だかscePowerVolatileMemLock()だかを使う方が正しそうだけど、
		なぜかこれらを使うとスレッド復帰時にフリーズしてしまう。
		
		第一引数にアクセス権を変更する先頭アドレス、第二引数に変更するサイズ。
		第三引数がアクセス権の設定で以下のフラグの論理和で渡すようだ。
		
		0x1 ユーザ読み込み許可
		0x2 ユーザ書き込み許可
		0x4 カーネル読み込み許可
		0x8 カーネル書き込み許可
		
		ここでは念のため0xFを渡して全フラグを許可した。
	*/
	sceKernelSetDdrMemoryProtection( (void *)0x88400000, 0x400000, 0xF );
	
	/* 拡張メモリから領域を確保 */
	internal_frame_buf = mfKernelVolatileMemAlloc( frame_buf_size );
	clear_raw_image    = mfKernelVolatileMemAlloc( frame_buf_size );
	return_raw_image   = mfKernelVolatileMemAlloc( frame_buf_size ); //贅沢に!!
	
	/* 終了時用背景をメモリの確保に成功していれば保存 */
	if( return_raw_image ){
		memcpy( return_raw_image, backup_fb.frameBuffer, frame_buf_size );
	}
	
	/* 表示バッファにMFM_BG_COLORをのせる */
	blitFillRect( 0, 0, SCR_WIDTH, SCR_HEIGHT, MFM_BG_COLOR );
	blitFillRect( 0, 0, SCR_WIDTH, blitOffsetLine( 3 ), MFM_TITLE_BAR_COLOR );
	blitFillRect( 0, SCR_HEIGHT - blitOffsetLine( 3 ), SCR_WIDTH, SCR_HEIGHT, MFM_INFO_BAR_COLOR );
	blitString( blitOffsetChar( 3 ), blitOffsetLine( 1 ), MFM_TITLE_TEXT_COLOR, MFM_TRANSPARENT, MFM_TOP_MESSAGE );
	
	/* 拡張メモリを確保できていたかをチェック */
	if( ! clear_raw_image || ! internal_frame_buf ){
		out_of_memory = true;
		goto DESTROY;
	}
	
	/* クリア用背景を保存 */
	memcpy( clear_raw_image, backup_fb.frameBuffer, frame_buf_size );
	
	/* フレームバッファマネージャをinternal_frame_bufを二枚目のフレームとして初期化 */
	fbmgrInit( internal_frame_buf );
	
	/* blitterの描画アドレスを描画バッファへ */
	blitSetFrameBufAddrTop( internal_frame_buf );
	
	/* 基本メニューを作成する */
	menu_num = mftableEntry + MFM_MAIN_MENU_COUNT;
	menu     = (MfMenuItem *)memsceMalloc( sizeof( MfMenuItem ) * menu_num );
	if( ! menu ){
		out_of_memory = true;
		goto DESTROY;
	}
	mf_menu_create_base_menu( menu, menu_num, &mf_menu );
	
	/* パッドのキーリピート計算機を初期化 */
	ctrlpadInit( &st_cp_params );
	ctrlpadSetRepeatButtons( &st_cp_params, PSP_CTRL_UP | PSP_CTRL_RIGHT | PSP_CTRL_DOWN | PSP_CTRL_LEFT );
	
	mfMenuEnableInterrupt();
	
	while( gRunning ){
		st_pad_data.Buttons = ctrlpadGetData( &st_cp_params, &st_pad_data );
		
		if( st_interrupt && st_pad_data.Buttons & ( PSP_CTRL_START | PSP_CTRL_HOME ) ){
			st_menu_quit = true;
		}
		
		if( st_menu_quit ){
			
			st_menu_quit = false;
			break;
		}
		
		/* 背景を描画 */
		memcpy( fbmgrGetCurrentDrawBuf(), clear_raw_image, frame_buf_size );
		
		/* インジケータを描画 */
		mf_indicator( &indicator );
		
		if( rc == MR_BACK ){
			mfMenuQuit();
		} else if( rc == MR_CONTINUE ){
			rc = mfMenuUniDraw( blitOffsetChar( 5 ), blitOffsetLine( 4 ), menu, menu_num, &menu_selected, 0 );
		} else{
			MfMenuRc func_rc = ( (MfFuncMenu)mf_menu.func )( &st_pad_data, mf_menu.arg );
			if( func_rc == MR_BACK ) rc = MR_CONTINUE;
		}
		
		/* 表示バッファと描画バッファを切り替えて、blitterの描画アドレスを新しい描画バッファへ */
		blitSetFrameBufAddrTop( fbmgrSwapBuffers() );
		sceDisplayWaitVblankStart();
		
		if( st_wait_micro_sec ){
			sceKernelDelayThread( st_wait_micro_sec );
			st_wait_micro_sec = 0;
		}
	}
	
	goto DESTROY;
	
	DESTROY:
		/* メモリ不足だった場合は緊急用の切り替えボタンをチェック */
		if( out_of_memory ){
			blitSetFrameBufAddrTop( backup_fb.frameBuffer );
			mf_not_enough_mem_error();
		} else{
			if( clear_raw_image ){
				memcpy( fbmgrGetCurrentDrawBuf(), clear_raw_image, frame_buf_size );
				mfKernelVolatileMemFree( clear_raw_image );
				clear_raw_image = NULL;
			} else{
				memset( fbmgrGetCurrentDrawBuf(), 0, frame_buf_size );
			}
			blitString( blitOffsetChar( 30 ), blitOffsetLine( 17 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Returning to the game..." );
			fbmgrSwapBuffers();
			sceKernelDelayThread( 300000 );
			
			if( return_raw_image ){
				memcpy( backup_fb.frameBuffer, return_raw_image, frame_buf_size );
				mfKernelVolatileMemFree( return_raw_image );
				return_raw_image = NULL;
			} else{
				memset( backup_fb.frameBuffer, 0, frame_buf_size );
			}
		}
		
		/* メニューリストを解放 */
		if( menu ){
			memsceFree( menu );
		}
		
		/* 内部フレームバッファを解放 */
		if( internal_frame_buf ){
			/* フレームバッファマネージャを破棄し、表示バッファを元の位置に戻す */
			fbmgrDestroy();
			
			mfKernelVolatileMemFree( internal_frame_buf );
			internal_frame_buf = NULL;
		}
		
		
		/* APIがフックされていたのならば再度フック */
		if( mf_api_hooked ) mfHookApi();
		
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

MfMenuRc mfMenuMultiDraw( int x, int y, MfMenuItem **menu, int row, int col, int *select_row, int *select_col, MfMenuOptions options )
{
	/* 作りかけ。まだ使うところがないのでとりあえず放置 
	int current_row, current_col, line_x, line_y;
	char line[MFM_ITEM_LENGTH];
	
	if( *select_row < 0 ){
		*select_row = 0;
	} else if( *select_row >= row ){
		*select_row = row - 1;
	}
	
	if( *select_col < 0 ){
		*select_col = 0;
	} else if( *select_col >= col ){
		*select_col = col - 1;
	}
	
	for( current_col = 0; current_col < col; current_col++ ){
		line_x = x;
		line_y = y + blitOffsetLine( current_col );
		for( current_row = 0; current_row < row; current_row++ ){
			if( menu[current_col][current_row].type == MT_NULL ) continue;
			
			mf_menu_create_item_name( line, sizeof( line ), &(menu[current_col][current_row]) );
			
			if( current_col == *select_col && current_row == *select_row ){
				blitString( line_x, line_y, MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, line );
			} else{
				blitString( line_x, line_y, MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, line );
			}
			line_x += menu[current_col][current_row].width;
		}
	}
	*/
	return MR_CONTINUE;
}

MfMenuRc mfMenuUniDraw( int x, int y, MfMenuItem menu[], size_t max, int *select, MfMenuOptions options )
{
	int current;
	char line[MFM_ITEM_LENGTH];
	
	if( *select < 0 ){
		*select = 0;
	} else if( *select >= max ){
		*select = max - 1;
	}
	
	for( current = 0; current < max; current++ ){
		if( menu[current].type == MT_NULL ) continue;
		
		/* タイプごとにメニュー項目文字列作成 */
		mf_menu_create_item_name( line, sizeof( line ), &(menu[current]) );
		
		/* メニュー項目 */
		blitString( x, y + blitOffsetLine( current ), current == *select ? MFM_TEXT_FCCOLOR : MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, line );
	}
	
	if( options & MO_DISPLAY_ONLY ) return MR_CONTINUE;
	
	if( ! st_dialog ){
		/* 操作方法 */
		blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ) - ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x80\x82 = Move, \x86 = Back, START/HOME = Exit" );
		switch( menu[*select].type ){
			case MT_BOOL:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ) + ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x83\x81 = Toggle switch" );
				break;
			case MT_OPTION:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ) + ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x83\x81 = Change option" );
				break;
			case MT_GET_NUMBERS:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ) + ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x83 = Decrease, \x81 = Increase, \x85 = Edit" );
				break;
			case MT_GET_BUTTONS:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ) + ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x85 = Edit" );
				break;
			default:
				blitString( blitOffsetChar( 3 ), blitOffsetLine( 32 ) + ( BLIT_CHAR_HEIGHT >> 1 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "\x85 = Enter" );
				break;
		}
		
		if( st_pad_data.Buttons & PSP_CTRL_UP ){
			do{
				(*select)--;
				if( *select < 0 ) *select = max - 1;
			} while( menu[*select].type == MT_NULL );
		} else if( st_pad_data.Buttons & PSP_CTRL_DOWN ){
			do{
				(*select)++;
				if( *select >= max ) *select = 0;
			} while( menu[*select].type == MT_NULL );
		} else if( st_pad_data.Buttons & PSP_CTRL_CROSS ){
			return MR_BACK;
		}
		return mf_menu_control( &st_pad_data, &(menu[*select]) );
	} else{
		if( mfMenuGetNumberIsReady() ){
			MfMenuRc rc = mfMenuGetNumber( NULL );
			if( rc != MR_CONTINUE ) st_dialog = false;
		} else if( mfMenuGetButtonsIsReady() ){
			MfMenuRc rc = mfMenuGetButtons( NULL );
			if( rc != MR_CONTINUE ) st_dialog = false;
		}
		
		return MR_CONTINUE;
	}
}

static MfMenuRc mf_menu_control( SceCtrlData *pad_data, MfMenuItem *item )
{
	if( ! pad_data->Buttons ){
	/* 何も押されていなければすぐ戻る */
		return MR_CONTINUE;
	} else if( pad_data->Buttons & PSP_CTRL_LEFT ){
	/* 左が押された */
		if( item->type == MT_BOOL ){
			*((bool *)item->handler) = *((bool *)item->handler) ? false : true;
		} else if( item->type == MT_OPTION ){
			if( *((int *)item->handler) == 0 ){
				for( ; item->value[*((int *)item->handler) + 1].integer; (*((int *)item->handler))++ );
			} else{
				(*((int *)item->handler))--;
			}
		} else if( item->type == MT_GET_NUMBERS ){
			if( *((int *)item->handler) > 0 ) (*((int *)item->handler))--;
		}
	} else if( pad_data->Buttons & PSP_CTRL_RIGHT ){
	/* 右が押された */
		if( item->type == MT_BOOL ){
			*((bool *)item->handler) = *((bool *)item->handler) ? false : true;
		} else if( item->type == MT_OPTION ){
			if( item->value[*((int *)item->handler) + 1].pointer == NULL ){
				*((int *)item->handler) = 0;
			} else{
				(*((int *)item->handler))++;
			}
		} else if( item->type == MT_GET_NUMBERS ){
			(*((int *)item->handler))++;
		}
	} else if( pad_data->Buttons & PSP_CTRL_CIRCLE ){
	/* ○が押された */
		if( item->type == MT_ANCHOR ){
			return MR_ENTER;
		} else if( item->type == MT_CALLBACK ){
			((MfMenuCallback *)(item->handler))->func = item->value[0].pointer;
			((MfMenuCallback *)(item->handler))->arg  = item->value[1].pointer;
			return MR_ENTER;
		} else if( item->type == MT_GET_NUMBERS ){
			MfMenuRc rc = mfMenuGetNumberInit(
				item->value[0].string,
				item->value[1].string,
				(long *)(item->handler),
				item->value[2].integer
			);
			
			if( rc != MR_ENTER ) return MR_CONTINUE;
			
			st_dialog = true;
		} else if( item->type == MT_GET_BUTTONS ){
			MfMenuRc rc = mfMenuGetButtonsInit(
				item->value[0].string,
				(unsigned int *)(item->handler),
				(unsigned int)(item->value[1].integer)
			);
			
			if( rc != MR_ENTER ) return MR_CONTINUE;
			
			st_dialog = true;
		}
	}
	
	return MR_CONTINUE;
}

static void mf_menu_create_item_name( char line[], size_t max, MfMenuItem *item )
{
	switch( item->type ){
		case MT_BOOL:
			snprintf( line, max, "%s: %s", item->label, ( *((bool *)item->handler) ? item->value[1].string : item->value[0].string ) );
			break;
		case MT_OPTION:
			snprintf( line, max, "%s: %s", item->label, item->value[*((int *)item->handler)].string );
			break;
		case MT_GET_NUMBERS:
			snprintf( line, max, "%s: %d %s", item->label, *((int *)item->handler), item->value[1].string );
			break;
		default:
			strutilSafeCopy( line, item->label, max );
	}
}

static void mf_not_enough_mem_error( void )
{
	blitString( blitOffsetChar( 5 ), blitOffsetLine(  4 ), MFM_TEXT_FCCOLOR, MFM_TEXT_BGCOLOR, "Failed to allocate the extra 4MB of RAM." );
	blitString( blitOffsetChar( 5 ), blitOffsetLine(  6 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Maybe you are running Sony's Save/Load dialog in the game side." );
	blitString( blitOffsetChar( 5 ), blitOffsetLine(  7 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Please exit there." );
	blitString( blitOffsetChar( 5 ), blitOffsetLine(  8 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Otherwise, there is not enough available memory." );
	
	blitString( blitOffsetChar( 5 ), blitOffsetLine( 10 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Waiting for button press..." );
	blitString( blitOffsetChar( 9 ), blitOffsetLine( 11 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "START/HOME = Quit." );
	blitString( blitOffsetChar( 9 ), blitOffsetLine( 12 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "SELECT     = Toggling MacroFire Engine and quit." );
	
	blitStringf( blitOffsetChar( 5 ), blitOffsetLine( 14 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Currently MacroFire Engine: %s", gMfEngine ? "ON" : "OFF" );
	
	do{
		sceCtrlPeekBufferPositive( &st_pad_data, 1 );
		sceKernelDelayThread( 10000 );
	} while( ! ( st_pad_data.Buttons & ( PSP_CTRL_START | PSP_CTRL_SELECT | PSP_CTRL_HOME ) ) );
	
	if( st_pad_data.Buttons & ( PSP_CTRL_START | PSP_CTRL_HOME ) ){
		blitStringf( blitOffsetChar( 5 ), blitOffsetLine( 16 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "Quit." );
	} else if( st_pad_data.Buttons & PSP_CTRL_SELECT ){
		if( mfIsEnabled() ){
			 mfDisable();
		} else{
			mfEnable();
		}
		blitStringf( blitOffsetChar( 5 ), blitOffsetLine( 16 ), MFM_TEXT_FGCOLOR, MFM_TEXT_BGCOLOR, "MacroFire Engine is toggled.\nQuit." );
	}
	
	sceKernelDelayThread( 2000000 );
}

static void mf_menu_create_base_menu( MfMenuItem *menu, int menu_num, MfMenuCallback *mf_menu )
{
	int mftable_index;
	
	menu[0].type            = MT_BOOL;
	menu[0].label           = "MacroFire Engine";
	menu[0].width           = 0;
	menu[0].handler         = &gMfEngine;
	menu[0].value[0].string = "OFF";
	menu[0].value[1].string = "ON";
	
	menu[1].type             = MT_NULL;
	menu[1].label            = NULL;
	menu[1].width            = 0;
	menu[1].handler          = NULL;
	menu[1].value[0].pointer = NULL;
	
	menu[2].type             = MT_GET_BUTTONS;
	menu[2].label            = "MacroFire toggle buttons";
	menu[2].width            = 0;
	menu[2].handler          = &gMfToggle;
	menu[2].value[0].string  = "Set MacroFire toggle buttons";
	menu[2].value[1].integer = (
		PSP_CTRL_CIRCLE   | PSP_CTRL_CROSS    | PSP_CTRL_SQUARE   | PSP_CTRL_TRIANGLE |
		PSP_CTRL_UP       | PSP_CTRL_RIGHT    | PSP_CTRL_DOWN     | PSP_CTRL_LEFT     |
		PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT   | PSP_CTRL_START    |
		PSP_CTRL_NOTE     | PSP_CTRL_SCREEN   | PSP_CTRL_VOLUP    | PSP_CTRL_VOLDOWN
	);
	
	menu[3].type             = MT_NULL;
	menu[3].label            = NULL;
	menu[3].width            = 0;
	menu[3].handler          = NULL;
	menu[3].value[0].pointer = NULL;
	
	for( mftable_index = MFM_MAIN_MENU_COUNT; mftable_index < menu_num; mftable_index++ ){
		menu[mftable_index].type     = MT_CALLBACK;
		menu[mftable_index].label    = mftable[mftable_index - MFM_MAIN_MENU_COUNT].name;
		menu[mftable_index].width    = 0;
		menu[mftable_index].handler  = mf_menu;
		menu[mftable_index].value[0].pointer = mftable[mftable_index - MFM_MAIN_MENU_COUNT].menu.func;
		menu[mftable_index].value[1].pointer = mftable[mftable_index - MFM_MAIN_MENU_COUNT].menu.arg;
	}
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
	params->ui.x                = blitOffsetChar( 39 );
	params->ui.y                = blitOffsetLine( 5 );
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

MfMenuRc mfMenuGetNumber( long *number )
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
		
		if( number ){
			*number = *(params->data->numSave);
		}
		
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
	params->ui.x                   = blitOffsetChar( 39 );
	params->ui.y                   = blitOffsetLine( 5 );
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

MfMenuRc mfMenuGetButtons( unsigned int *buttons )
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
		
		if( buttons ){
			*buttons = *(params->data->buttonsSave);
		}
		
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

MfMenuRc mfMenuGetFilenameInit( const char *title, unsigned int flags, const char *initpath, char *path, size_t pathmax, char *name, size_t namemax )
{
	CmndlgGetFilenameParams *params;
	
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
	
	params->numberOfData         = 1;
	params->selectDataNumber     = 0;
	params->rc                   = 0;
	params->ui.x                 = blitOffsetChar( 0 );
	params->ui.y                 = blitOffsetLine( 3 );
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
	params->data->path           = path;
	params->data->pathMax        = pathmax;
	params->data->name           = name;
	params->data->nameMax        = namemax;
	params->data->path[0]        = '\0';
	params->data->name[0]        = '\0';
	
	if( cmndlgGetFilenameStart( params ) ){
		cmndlgGetFilenameShutdownStart();
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
		CmndlgRc rc;
		CmndlgGetFilenameParams *params = cmndlgGetFilenameGetParams();
		
		rc = params->rc;
		
		cmndlgGetFilenameShutdownStart();
		
		if( path ){
			*path = params->data->path;
		}
		if( name ){
			*name = params->data->name;
		}
		
		memsceFree( params->data );
		memsceFree( params );
		mfMenuKeyRepeatReset();
		mfMenuEnableInterrupt();
		return rc == CMNDLG_ACCEPT ? MR_ENTER : MR_BACK;
	}
	
	return MR_CONTINUE;
}

