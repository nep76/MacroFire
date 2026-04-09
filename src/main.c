/*
	MacroFire
*/

#include "main.h"
#include "hooktable.h"

PSP_MODULE_INFO( "MacroFire", PSP_MODULE_KERNEL, 0, 0 );

static SceCtrlData  st_pad_data;

static int  st_sampling_mode = 100;
static bool st_apihooked = false;
static bool st_wait_toggle_button_release = false;
static unsigned int st_prev_pad_buttons;

static void mf_call_intr( const int mfengine )
{
	int i;
	for( i = 0; i < mftableEntry; i++ ){
		if( mftable[i].intrFunc ) ( mftable[i].intrFunc )( mfengine );
	}
}

static int mf_read_pad_buffer( MfSceCtrlDataFunc func, SceCtrlData *pad, int count, MfCallMode mode )
{
	/*
		K1レジスタのメモ
		mf_read_pad_buffer()は内部で第1引数に指定された本来のAPIを実行する。
		しかし本来のコードは、ファームウェア内部のOSカーネル専用領域に存在する。
		ここにアクセスするには、特権モードに移行しなければならない。
		
		普通にAPIはシステムコール例外の例外ハンドラとして動いているようで、
		APIをコールするとSYSCALL命令が発行され、例外ハンドラを実行する際に
		特権モードへ移行するという仕掛けを使って実行できるようになっているっぽい？
		
		pspSdkSetK1( 0 )を実行すると特権モードに移行させられるようだ。
		つまり、これを行ったあとならばファームウェア内のコードを直接実行できる。
		
		やることやったら元の値に戻せるように、実行前にpspSdkGetK1()で値を保存しておく。
	*/
	int ret = ( func )( pad, count );
	if( mode != MF_CALL_INTERNAL ){
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].hook.func ) ( mftable[i].hook.func )( mode, pad, mftable[i].hook.arg );
		}
	}
	
	return ret;
}

int mfCtrlPeekBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[0].syscall.func, pad, count, MF_CALL_READ );
}

int mfCtrlPeekBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( Hooktable[0].syscall.func, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlReadBufferPositive( SceCtrlData *pad, int count )
{
	return mf_read_pad_buffer( Hooktable[2].syscall.func, pad, count, MF_CALL_READ );
}

int mfCtrlReadBufferNegative( SceCtrlData *pad, int count )
{
	int ret = mf_read_pad_buffer( Hooktable[2].syscall.func, pad, count, MF_CALL_READ );
	
	pad->Buttons = ~pad->Buttons;
	
	return ret;
}

int mfCtrlPeekLatch( SceCtrlLatch *latch )
{
	SceCtrlData pad;
	int ret;
	unsigned int k1 = pspSdkGetK1();
	pspSdkSetK1( 0 );
	
	ret = mf_read_pad_buffer( Hooktable[0].syscall.func, &pad, 1, MF_CALL_LATCH );
	
	pspSdkSetK1( k1 );
	
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
	int ret;
	unsigned int k1 = pspSdkGetK1();
	pspSdkSetK1( 0 );
	
	ret = mf_read_pad_buffer( Hooktable[0].syscall.func, &pad, 1, MF_CALL_LATCH );
	
	pspSdkSetK1( k1 );
	
	latch->uiMake    = ( st_prev_pad_buttons ^ pad.Buttons ) & pad.Buttons;
	latch->uiBreak   = ( st_prev_pad_buttons ^ pad.Buttons ) & st_prev_pad_buttons;
	latch->uiPress   = pad.Buttons;
	latch->uiRelease = ~pad.Buttons;
	
	st_prev_pad_buttons = pad.Buttons;
	
	return ret;
}

void mfHookApi( void )
{
	int i;
	
	if( st_apihooked ) return;
	
	for( i = 0; i < ARRAY_NUM( Hooktable ); i++ ){
		hookApiByNid(
			&(Hooktable[i].syscall),
			Hooktable[i].modname,
			Hooktable[i].library,
			Hooktable[i].nid,
			Hooktable[i].hookfunc
		);
	}
	
	st_apihooked = true;
}

void mfRestoreApi( void )
{
	int i;
	
	if( ! st_apihooked ) return;
	
	for( i = 0; i < ARRAY_NUM( Hooktable ); i++ ){
		hookRestoreAddr( &(Hooktable[i].syscall) );
	}
	
	st_apihooked = false;
}

int main_thread( SceSize arglen, void *argp )
{
	/* メニューを初期化 */
	mfMenuInit();
	
	{
		chdir( "ms0:" );
		
		/* 初期化関数呼び出し */
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].initFunc ) ( mftable[i].initFunc )();
		}
	}
	
	while( gRunning ){
		/* module_stop()が呼ばれないので、一時停止をして強制終了可能な状態にする */
		sceKernelDelayThread( 50000 );
		
		if( st_apihooked ){
			unsigned int k1 = pspSdkGetK1();
			pspSdkSetK1( 0 );
			mf_read_pad_buffer( Hooktable[0].syscall.func, &st_pad_data, 1, MF_CALL_INTERNAL );
			pspSdkSetK1( k1 );
		} else{
			sceCtrlPeekBufferPositive( &st_pad_data, 1 );
		}
		
		if( gMfToggle ){
			/* 検出する必要のないボタンフラグを消す */
			st_pad_data.Buttons ^= ( st_pad_data.Buttons & ( PSP_CTRL_WLAN_UP | PSP_CTRL_REMOTE | PSP_CTRL_DISC | PSP_CTRL_MS ) );
			
			if( st_pad_data.Buttons & gMfToggle ){
				if( ! st_wait_toggle_button_release ){
					gMfEngine = ( gMfEngine == MFENGINE_ON ? MFENGINE_OFF : MFENGINE_ON );
					st_wait_toggle_button_release = true;
				}
			} else if( st_wait_toggle_button_release ){
				st_wait_toggle_button_release = false;
			}
		}
		
		if( ! st_apihooked && gMfEngine ){
			/* APIをフック */
			mfHookApi();
			mf_call_intr( gMfEngine );
		} else if( st_apihooked && ! gMfEngine ){
			/* APIをリストア */
			mfRestoreApi();
			mf_call_intr( gMfEngine );
		}
		
		if( st_pad_data.Buttons & PSP_CTRL_VOLUP && st_pad_data.Buttons & PSP_CTRL_VOLDOWN ) mfMenu();
	}
	
	/* メニュー終了処理 */
	//mfMenuTerm();
	
	{
		/* 終了関数呼び出し */
		int i;
		for( i = 0; i < mftableEntry; i++ ){
			if( mftable[i].termFunc ) ( mftable[i].termFunc )();
		}
	}
	
	return sceKernelExitDeleteThread( 0 );
}

bool mfIsApiHooked( void )
{
	return st_apihooked;
}

void mfEnable( void )
{
	gMfEngine = MFENGINE_ON;
}

void mfDisable( void )
{
	gMfEngine = MFENGINE_OFF;
}

bool mfIsEnabled( void )
{
	return ( gMfEngine == 0 ? false : true );
}

bool mfIsDisabled( void )
{
	return ( gMfEngine == 0 ? true : false );
}

int module_start( SceSize arglen, void *argp )
{
	SceUID thid;
	
	thid = sceKernelCreateThread( "MacroFire", main_thread, 32, 0x1500, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
	if( thid ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	/* 呼び出されていない。どうすればいいのか？ */
	
	int i;
	gRunning = false;
	
	if( ! mfIsApiHooked() ) return 0;
	
	for( i = 0; i < ARRAY_NUM( Hooktable ); i++ ){
		hookRestoreAddr( &(Hooktable[i].syscall) );
	}
	
	return 0;
}
