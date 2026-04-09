/*
	menu.c
*/

#include "menu.h"
#include "mftable.h"

static int  st_wait        = 0;
static bool st_clear_vsync = false;
static bool st_mfmenu_quit = false;
static bool st_interrupt   = true;
static SCE_CTRL_LATCH_FUNC st_f_ctrlReadLatch;
static SCE_CTRL_DATA_FUNC  st_f_ctrlPeekBufferPositive;
static SceCtrlLatch        *st_pad_latch;
static SceCtrlData         *st_pad_data;
static SceUID              st_thlist_first[MAX_NUMBER_OF_THREADS];
static int                 st_thnum_first;

bool mfMenuInit( void )
{
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, st_thlist_first, MAX_NUMBER_OF_THREADS, &st_thnum_first );
	return true;
}

void mfMenuTerm( void )
{
}

void mfMenuSetup( SCE_CTRL_LATCH_FUNC ctrlReadLatch, SCE_CTRL_DATA_FUNC ctrlPeekBufferPositive, SceCtrlLatch *pad_latch, SceCtrlData *pad_data )
{
	st_f_ctrlReadLatch          = ctrlReadLatch;
	st_f_ctrlPeekBufferPositive = ctrlPeekBufferPositive;
	st_pad_latch                = pad_latch;
	st_pad_data                 = pad_data;
}

void mfClearColor( u32 color )
{
	blitFillRect( 0, 0, 480, 272, color );
}

void mfMenuQuit( void )
{
	st_mfmenu_quit = true;
}

void mfMenu( void )
{
	SceUID thlist[MAX_NUMBER_OF_THREADS];
	int thnum, selected = 0;
	
	MfFuncMenu function = NULL;
	
	MfMenuItem *miList;
	int miCount, i;
	
	sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MAX_NUMBER_OF_THREADS, &thnum );
	mfThreadsStatChange( false, thlist, thnum );
	
	/* blitの8bitASCIIテーブルをPSPボタンシンボルへ */
	blit8BitCharTableSwitch( B8_BUTTON_SYMBOL );
	
	/* "+ 2"は、先頭の"MacroFire Engine"のオプションとその次のMT_BORDERの分 */
	miCount = mftableEntry + 2;
	miList  = (MfMenuItem *)memsceMallocEx( 16, PSP_MEMPART_KERNEL_1, "miList", PSP_SMEM_Low, sizeof( MfMenuItem ) * miCount, 0 );
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
	
	for( i = 2; i < miCount; i++ ){
		miList[i].type     = MT_ANCHOR;
		miList[i].selected = NULL;
		miList[i].label    = mftable[i - 2].menu.label;
		miList[i].value[0] = NULL;
	}
	
	mfClearColor( MENU_BGCOLOR );
	
	while( gRunning ){
		( st_f_ctrlReadLatch )( st_pad_latch );
		( st_f_ctrlPeekBufferPositive )( st_pad_data, 1 );
		
		if(
			( st_interrupt && ( st_pad_latch->uiMake & PSP_CTRL_START || st_pad_latch->uiMake & PSP_CTRL_HOME ) ) ||
			st_mfmenu_quit ||
			( ! function && st_pad_latch->uiMake & PSP_CTRL_CROSS )
		){
			st_mfmenu_quit = false;
			mfClearColor( 0 );
			break;
		}
		
		blitString( 0, 0, MENU_FGCOLOR, MENU_BGCOLOR, MF_MENU_TOP_MESSAGE );
		blitLine( 0, 10, 480, 10, MENU_FGCOLOR );
		blitLine( 0, blitOffsetLine( 31 ) - 5, 480, blitOffsetLine( 31 ) - 5, MENU_FGCOLOR );
		
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
						"\x80 = MoveUp, \x82 = MoveDown, \x85 = Enter or Change toggle\n\x86 or START = Exit"
					);
				case MR_BACK:
					break;
				case MR_ENTER:
					mfClearColor( MENU_BGCOLOR );
					function = mftable[selected - 2].menu.mainFunc;
					break;
			}
		} else{
			if( ! ( function )( st_pad_latch, st_pad_data, mftable[selected - 2].menu.arg ) ){
				mfClearColor( MENU_BGCOLOR );
				function = NULL;
			}
		}
		
		sceDisplayWaitVblankStart();
		sceKernelDcacheWritebackAll();
		
		if( st_wait ){
			sceKernelDelayThread( st_wait * 1000000 );
			st_wait = 0;
		}
		if( st_clear_vsync ){
			mfClearColor( MENU_BGCOLOR );
			st_clear_vsync = false;
		}
		
	}
	
	memsceFree( miList );
	goto QUIT;
	
	QUIT:
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
}

MfMenuReturnCode mfMenuVertical( int x, int y, int w, MfMenuItem mi[], size_t items_num, int *selected )
{
	int i;
	char line[MENUITEM_LENGTH];
	
	/* 選択項目が0未満になっている場合は0に初期化 */
	if( *selected < 0 ) *selected = 0;
	
	if( st_pad_latch->uiMake & PSP_CTRL_UP ){
		mfMenuMoveFocus( MM_PREV, mi, items_num, selected );
	} else if( st_pad_latch->uiMake & PSP_CTRL_DOWN ){
		mfMenuMoveFocus( MM_NEXT, mi, items_num, selected );
	} else if( st_pad_latch->uiMake & PSP_CTRL_CIRCLE ){
		if( mi[*selected].type == MT_OPTION ){
			if( mi[*selected].value[(*mi[*selected].selected) + 1] != 0 ){
				(*mi[*selected].selected) += 1;
			} else{
				(*mi[*selected].selected) = 0;
			}
			blitFillBox( x, y + ( *selected * BLIT_CHAR_HEIGHT ), w, BLIT_CHAR_HEIGHT, MENU_BGCOLOR );
		} else{
			return MR_ENTER;
		}
	} else if( st_pad_latch->uiMake & PSP_CTRL_CROSS ){
		return MR_BACK;
	}
	
	for( i = 0; i < items_num; i++ ){
		if( mi[i].type != MT_BORDER ){
			safe_strncpy( line, mi[i].label, MENUITEM_LENGTH );
			if( mi[i].type == MT_OPTION )
				snprintf( line, MENUITEM_LENGTH, "%s: %s", line, mi[i].value[(*mi[i].selected)] );
			
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

bool mfIsEnabled( void )
{
	return ( gMfEngine == 0 ? false : true );
}

bool mfIsDisabled( void )
{
	return ( gMfEngine == 0 ? true : false );
}

void mfMenuEnableInterrupt( void )
{
	st_interrupt = true;
}

void mfMenuDisableInterrupt( void )
{
	st_interrupt = false;
}
