/*
	menu.c
*/

#include "menu.h"
#include "mftable.h"

static int  st_wait        = 0;
static bool st_clear_vsync = false;
static bool st_mfmenu_quit = false;
static bool st_interrupt   = true;
static SceCtrlData         st_pad_data;
static SceCtrlLatch        st_pad_latch;
static SceUID              st_thlist_first[MAX_NUMBER_OF_THREADS];
static int                 st_thnum_first;

static void mf_menu_get_digits( MfMenuItem *mi )
{
	CmndlgGetDigitsData cgd[1];
	long delay = *(mi->selected);
	
	cgd[0].title     = mi->value[0];
	cgd[0].unit      = mi->value[1];
	cgd[0].number    = &delay;
	cgd[0].numDigits = strtol( mi->value[2], NULL, 10 );
	
	cmndlgGetDigits( blitOffsetChar( 39 ), blitOffsetLine( 5 ), cgd, 1 );
	mfClearColor( MENU_BGCOLOR );
	
	*(mi->selected) = (int)delay;
}

static void mf_menu_get_buttons( MfMenuItem *mi )
{
	//使うところがいまのところないのでそのうち
	//受け取るとき、MfMenuItemのselectedメンバがint型なのでunsigned int型にキャストする
}

bool mfMenuInit( void )
{
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, st_thlist_first, MAX_NUMBER_OF_THREADS, &st_thnum_first );
	return true;
}

void mfClearColor( u32 color )
{
	blitFillRect( 0, 0, 480, 272, color );
}

void mfDrawMainFrame( void )
{
	blitString( 0, 0, MENU_FGCOLOR, MENU_BGCOLOR, MF_MENU_TOP_MESSAGE );
	blitLine( 0, 10, 480, 10, MENU_FGCOLOR );
	blitLine( 0, blitOffsetLine( 31 ) - 5, 480, blitOffsetLine( 31 ) - 5, MENU_FGCOLOR );
}

void mfMenuQuit( void )
{
	st_mfmenu_quit = true;
}

void mfMenu( void )
{
	SceUID thlist[MAX_NUMBER_OF_THREADS];
	int thnum, selected = -1;
	MfFuncMenu function = NULL;
	MfMenuItem *miList;
	int miCount, i;
	bool mfengine = mfIsEnabled();
	
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MAX_NUMBER_OF_THREADS, &thnum );
	mfThreadsStatChange( false, thlist, thnum );
	
	/* キーフックを解除 */
	if( mfengine ) mfRestoreApi();
	
	/* blitの8bitASCIIテーブルをPSPボタンシンボルへ */
	blit8BitCharTableSwitch( B8_BUTTON_SYMBOL );
	
	/* MACRO_MENU_OFFSETは、先頭の"MacroFire Engine"のオプションとその次のMT_BORDERの分 */
	miCount = mftableEntry + MACRO_MENU_OFFSET;
	miList  = (MfMenuItem *)memsceMalloc( sizeof( MfMenuItem ) * miCount );
	if( ! miList ) goto QUIT;
	
	/* MacroFire Engine */
	miList[0].type     = MT_OPTION;
	miList[0].selected = &gMfEngine;
	miList[0].label    = "MacroFire Engine";
	miList[0].value[0] = "OFF";
	miList[0].value[1] = "ON";
	miList[0].value[2] = NULL;
	miList[1].type     = MT_BORDER;
	miList[1].selected = NULL;
	miList[1].label    = NULL;
	miList[1].value[0] = NULL;
	
	miList[2].type     = MT_ANCHOR;
	miList[2].selected = NULL;
	miList[2].label    = "MacroFire toggle buttons";
	miList[2].value[0] = NULL;
	miList[3].type     = MT_BORDER;
	miList[3].selected = NULL;
	miList[3].label    = NULL;
	miList[3].value[0] = NULL;
	
	for( i = MACRO_MENU_OFFSET; i < miCount; i++ ){
		miList[i].type     = MT_ANCHOR;
		miList[i].selected = NULL;
		miList[i].label    = mftable[i - MACRO_MENU_OFFSET].menu.label;
		miList[i].value[0] = NULL;
	}
	
	mfClearColor( MENU_BGCOLOR );
	
	/* わざと一度無駄にラッチデータを取得してゲーム側からの入力を捨てる */
	sceCtrlReadLatch( &st_pad_latch );
	
	while( gRunning ){
		sceCtrlReadLatch( &st_pad_latch );
		sceCtrlPeekBufferPositive( &st_pad_data, 1 );
		
		if(
			( st_interrupt && ( st_pad_latch.uiMake & PSP_CTRL_START || st_pad_latch.uiMake & PSP_CTRL_HOME ) ) ||
			st_mfmenu_quit ||
			( ! function && st_pad_latch.uiMake & PSP_CTRL_CROSS )
		){
			st_mfmenu_quit = false;
			mfClearColor( 0 );
			break;
		}
		
		mfDrawMainFrame();
		
		if( ! function ){
			switch( mfMenuVertical( blitOffsetChar( 5 ), blitOffsetLine( 4 ), BLIT_SCR_WIDTH, miList, miCount, &selected ) ){
				case MR_CONTINUE:
					blitString(
						blitOffsetChar( 3 ),
						blitOffsetLine( 2 ),
						MENU_FGCOLOR,
						MENU_BGCOLOR,
						"Please choose a function."
					);
					blitString(
						blitOffsetChar( 3 ),
						blitOffsetLine( 31 ),
						MENU_FGCOLOR,
						MENU_BGCOLOR,
						"\x80\x82 = Move, \x83\x81 = Change toggle, \x85 = Enter, \x86 or START = Exit"
					);
					break;
				case MR_BACK:
					break;
				case MR_ENTER:
					if( selected == MACRO_MENU_BASECONF ){
						CmndlgGetButtonsData cgb[1];
						cgb[0].title   = "Set MacroFire toggle buttons";
						cgb[0].buttons = &gMfToggle;
						cgb[0].btnMask = 0;
						cmndlgGetButtons( blitOffsetChar( 40 ), blitOffsetLine( 2 ), cgb, 1 );
						mfClearColor( MENU_BGCOLOR );
					} else{
						mfClearColor( MENU_BGCOLOR );
						function = mftable[selected - MACRO_MENU_OFFSET].menu.mainFunc;
					}
					break;
			}
		} else{
			if( ! ( function )( &st_pad_latch, &st_pad_data, mftable[selected - 2].menu.arg ) ){
				mfClearColor( MENU_BGCOLOR );
				function = NULL;
			}
		}
		
		sceDisplayWaitVblankStart();
		sceKernelDcacheWritebackAll();
		
		if( st_wait ){
			sceKernelDelayThread( st_wait * 1000000 );
			st_wait = 0;
			/* わざと一度無駄にラッチデータを取得してウェイト中の入力を捨てる */
			sceCtrlReadLatch( &st_pad_latch );
		}
		if( st_clear_vsync ){
			mfClearColor( MENU_BGCOLOR );
			st_clear_vsync = false;
		}
		
	}
	
	memsceFree( miList );
	goto QUIT;
	
	QUIT:
		/* キーフックを戻す */
		if( mfengine ) mfHookApi();

		mfThreadsStatChange( true, thlist, thnum );
		
		/*
			リジュームされたスレッドがLCDを操作する時間待つ。
			待つ時間の根拠は薄く適当。
			画面を扱うスレッドはvsync周期で動いてるんじゃないか、という素人考え。
			というわけで、まるまる一周期分の処理を待つために、vsyncを二回待つ。
		*/
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		
		/* LCDを点灯 */
		sceDisplayEnable();
		return;
}

void mfMenuMoveFocus( MfMenuMoveFocusDir mvdir, MfMenuItem mi[], size_t items_num, int *selected )
{
	int i;
	int selected_dupe = *selected;
	
	if( mvdir == MM_PREV ){
		for( i = 1; *selected - i >= 0; i++ ){
			if( mi[*selected - i].label != 0 ){
				*selected -= i;
				break;
			}
		}
	} else if( mvdir == MM_NEXT ){
		for( i = 1; *selected + i < items_num; i++ ){
			if( mi[*selected + i].label != 0 ){
				*selected += i;
				break;
			}
		}
	}
	
	if( selected_dupe == *selected ){
		switch( mvdir ){
			case MM_PREV:
				*selected = items_num;
				mfMenuMoveFocus( MM_PREV, mi, items_num, selected );
				break;
			case MM_NEXT:
				*selected = -1;
				mfMenuMoveFocus( MM_NEXT, mi, items_num, selected );
				break;
		}
	}
}

MfMenuReturnCode mfMenuVertical( int x, int y, int w, MfMenuItem mi[], size_t items_num, int *selected )
{
	int i;
	char line[MENUITEM_LENGTH];
	
	/* 選択項目が0未満になっている場合は先頭を取得 */
	if( *selected < 0 ){
		*selected = -1;
		mfMenuMoveFocus( MM_NEXT, mi, items_num, selected );
	}
	
	if( st_pad_latch.uiMake & PSP_CTRL_UP ){
		mfMenuMoveFocus( MM_PREV, mi, items_num, selected );
	} else if( st_pad_latch.uiMake & PSP_CTRL_DOWN ){
		mfMenuMoveFocus( MM_NEXT, mi, items_num, selected );
	} else if( st_pad_latch.uiMake & PSP_CTRL_LEFT && mi[*selected].type == MT_OPTION ){
		if( (*mi[*selected].selected) == 0 ){
			for( ; mi[*selected].value[(*mi[*selected].selected) + 1]; (*mi[*selected].selected)++ );
		} else{
			(*mi[*selected].selected)--;
		}
		blitFillBox( x, y + ( *selected * BLIT_CHAR_HEIGHT ), w, BLIT_CHAR_HEIGHT, MENU_BGCOLOR );
	} else if( st_pad_latch.uiMake & PSP_CTRL_RIGHT && mi[*selected].type == MT_OPTION ){
		(*mi[*selected].selected)++;
		if( mi[*selected].value[(*mi[*selected].selected)] == NULL ) (*mi[*selected].selected) = 0;
		blitFillBox( x, y + ( *selected * BLIT_CHAR_HEIGHT ), w, BLIT_CHAR_HEIGHT, MENU_BGCOLOR );
	} else if( st_pad_latch.uiMake & PSP_CTRL_CIRCLE ){
		switch( mi[*selected].type ){
			case MT_ANCHOR:
				return MR_ENTER;
			case MT_GET_DIGITS:
				mf_menu_get_digits( &(mi[*selected]) );
				return MR_CONTINUE;
			case MT_GET_BUTTONS:
				mf_menu_get_buttons( &(mi[*selected]) );
				return MR_CONTINUE;
		}
	} else if( st_pad_latch.uiMake & PSP_CTRL_CROSS ){
		return MR_BACK;
	}
	
	for( i = 0; i < items_num; i++ ){
		if( mi[i].type != MT_BORDER ){
			safe_strncpy( line, mi[i].label, MENUITEM_LENGTH );
			if( mi[i].type == MT_OPTION ){
				snprintf( line, MENUITEM_LENGTH, "%s: %s", line, mi[i].value[(*mi[i].selected)] );
			} else if( mi[i].type == MT_GET_DIGITS ){
				snprintf( line, MENUITEM_LENGTH, "%s: %d", line, (*mi[i].selected) );
			}
			
			if( i == *selected ){
				blitString( x, y, MENU_FCCOLOR, MENU_BGCOLOR, line );
			} else{
				blitString( x, y, MENU_FGCOLOR, MENU_BGCOLOR, line );
			}
		}
		y += BLIT_CHAR_HEIGHT;
	}
	return MR_CONTINUE;
}

void mfThreadsStatChange( bool stat, SceUID thlist[], int thnum )
{
	int ( *request_stat_func )( SceUID ) = NULL;
	int i, j;
	SceUID selfid = sceKernelGetThreadId();
	
	if( stat ){
		request_stat_func = sceKernelResumeThread;
	} else{
		request_stat_func = sceKernelSuspendThread;
	}
	
	for( i = 0; i < thnum; i++ ){
		bool no_target = false;
		for( j = 0; j < st_thnum_first; j++ ){
			if( thlist[i] == st_thlist_first[j] || selfid == thlist[i] ){
				no_target = true;
				break;
			}
		}
		if( ! no_target ) ( *request_stat_func )( thlist[i] );
	}
}

void mfWaitScreenReload( int sec )
{
	st_wait = sec;
}

void mfClearScreenWhenVsync( void )
{
	st_clear_vsync = true;
}

void mfMenuEnableInterrupt( void )
{
	st_interrupt = true;
}

void mfMenuDisableInterrupt( void )
{
	st_interrupt = false;
}
