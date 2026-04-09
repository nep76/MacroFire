/*
	MacroFire
*/

#include "main.h"

PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 0, 0 );

static bool st_apihooked = false;
static unsigned int st_prev_pad_buttons;

static void mfKeyHook( HookCaller caller, SceCtrlData *pad )
{
	int i;
	
	for( i = 0; i < mftableHookEntry; i++ ){
		( mftableHook[i].func )( caller, pad, mftableHook[i].arg );
	}
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)st_hooktable[0].syscall.func )( pad, count );
	
	mfKeyHook( CALL_PEEK_BUFFER_POSITIVE, pad );
	
	return ret;
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)st_hooktable[1].syscall.func )( pad, count );
	
	mfKeyHook( CALL_PEEK_BUFFER_NEGATIVE, pad );
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)st_hooktable[2].syscall.func )( pad, count );
	
	mfKeyHook( CALL_READ_BUFFER_POSITIVE, pad );
	
	return ret;
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret = ( (SCE_CTRL_DATA_FUNC)st_hooktable[3].syscall.func )( pad, count );
	
	mfKeyHook( CALL_READ_BUFFER_NEGATIVE, pad );
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret = ( (SCE_CTRL_DATA_FUNC)st_hooktable[0].syscall.func )( &pad, 1 ); // 0x80000023が返る。なぜ？
	mfKeyHook( CALL_PEEK_LATCH, &pad );
	
	latch->uiMake    |= ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   |= ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   |= pad.Buttons;
	latch->uiRelease = ~0;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

int mfCtrlReadLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret = ( (SCE_CTRL_DATA_FUNC)st_hooktable[0].syscall.func )( &pad, 1 ); // 0x80000023が返る。なぜ？
	mfKeyHook( CALL_READ_LATCH, &pad );
	
	latch->uiMake    = ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   = ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   = pad.Buttons;
	latch->uiRelease = ~pad.Buttons;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

int main_thread( SceSize arglen, void *argp )
{
	int i;
	SceCtrlLatch pad_latch;
	SceCtrlData  pad_data;
	
	SCE_CTRL_LATCH_FUNC ctrlGetPadLatch = sceCtrlReadLatch;
	SCE_CTRL_DATA_FUNC  ctrlGetPadData  = sceCtrlPeekBufferPositive;
	
	/* 初期化関数呼び出し */
	for( i = 0; i < mftableInitEntry; i++ ){
		( mftableInit[i] )();
	}
	
	/* メニューを初期化 */
	mfMenuInit();
	mfMenuSetup( ctrlGetPadLatch, ctrlGetPadData, &pad_latch, &pad_data );
	
	while( gRunning ){
		sceKernelDelayThread( 50000 );
		if( ! st_apihooked && gMfEngine ){
			// APIをフック
			for( i = 0; i < ARRAY_NUM( st_hooktable ); i++ ){
				hookApiByNid(
					&(st_hooktable[i].syscall),
					st_hooktable[i].modname,
					st_hooktable[i].library,
					st_hooktable[i].nid,
					st_hooktable[i].hookfunc
				);
			}
			st_apihooked = true;
		} else if( st_apihooked && ! gMfEngine ){
			for( i = 0; i < ARRAY_NUM( st_hooktable ); i++ ){
				hookRestoreAddr( &(st_hooktable[i].syscall) );
			}
			st_apihooked = false;
		}
		
		( ctrlGetPadData )( &pad_data, 1 );
		( ctrlGetPadLatch )( &pad_latch );
		
		if( pad_data.Buttons & PSP_CTRL_VOLUP && pad_data.Buttons & PSP_CTRL_VOLDOWN ){
			mfMenu();
		}
	}
	
	/* メニュー終了処理 */
	mfMenuTerm();
	
	/* 終了関数呼び出し */
	for( i = 0; i < mftableTermEntry; i++ ){
		( mftableTerm[i] )();
	}
	
	return sceKernelExitDeleteThread( 0 );
}


int module_start( SceSize arglen, void *argp )
{
	SceUID thid;
	
	thid = sceKernelCreateThread( "MacroFire", main_thread, 15, 0x500, 0, 0 );
	if( thid ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( void )
{
	int i;
	gRunning = false;
	
	for( i = 0; i < ARRAY_NUM( st_hooktable ); i++ ){
		hookRestoreAddr( &(st_hooktable[i].syscall) );
	}
	
	return 0;
}
