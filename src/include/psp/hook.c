/*
	hook.c
*/

#include "hook.h"

void **hookFindExportAddr( const char *modname, const char *libname, unsigned int nid )
{
	SceModule *module;
	struct SceLibraryEntryTable *entry;
	unsigned int i = 0, *entrytable = NULL;
	
	/* ロード中のモジュールから必要なモジュールを検索 */
	module = sceKernelFindModuleByName( modname );
	if( ! module ) return NULL;
	
	/* 検索したモジュールから必要なライブラリを検索 */
	while( i < module->ent_size ){
		entry = (struct SceLibraryEntryTable *)( (unsigned int)(module->ent_top) + i );
		
		if( ( libname && ( strcmp( libname, entry->libname ) == 0 ) ) || ( ! libname && ! entry->libname ) ){
			entrytable = entry->entrytable;
			break;
		}
		
		i += entry->len * sizeof( uint32_t );
	}
	
	if( ! entrytable || ! entry->stubcount ) return NULL;
	
	/* ライブラリがエクスポートしているNIDの格納アドレスを返す */
	for( i = 0; i < entry->stubcount; i++ ){
		if( entrytable[i] == nid ) return (void **)&(entrytable[entry->vstubcount + entry->stubcount + i]);
	}
	
	return NULL;
}

void **hookFindSyscallAddr( void **addr )
{
	/*
		この関数は、NIDから導かれるメモリアドレスから
		実際にシステムコール例外ハンドラの位置を特定する。
		
		PSPLINKのコードを参考にしています。
	*/
	void **cop0_ctrl_register_12;
	struct hook_syscall_table_header *syscall_table;
	void **syscall_entry;
	unsigned int i, total_entries;
	
	if( ! addr ) return NULL;
	
	/*
		CFC0はコプロセッサ0の制御レジスタを読み込む命令のようだ。
		似た命令にMFC0があるようだが、こちらは汎用レジスタを読み込む命令らしく、別の値が返ってきた。
		
		このインラインアセンブラで、コプロセッサ0の12番目の制御レジスタをcop0_ctrl_register_12変数へコピーする。
		この12番目の制御レジスタにはシステムコールテーブルのアドレスがセットされている模様。
	*/
	asm( "cfc0 %0, $12;" : "=r"( cop0_ctrl_register_12 ) );
	
	/* cop0_ctrl_register_12を参照して、システムコールテーブルを取得 */
	syscall_table = *cop0_ctrl_register_12;
	
	/* システムコールテーブルのヘッダを飛ばして、エントリ部分を取得 */
	syscall_entry = (void **)( (unsigned int)syscall_table + sizeof( struct hook_syscall_table_header ) );
	
	/* ヘッダ部分を減算してテーブルの残りサイズをエントリ数で取得 */
	total_entries = ( syscall_table->size - sizeof( struct hook_syscall_table_header ) ) / sizeof( void* );
	
	/* 目的の例外ハンドラを検索し、アドレスを返す */
	for( i = 0; i < total_entries; i++ ){
		if( syscall_entry[i] == *addr ) return &(syscall_entry[i]);
	}
	
	return NULL;
}

void hookFunc( void **addr, void *hookfunc )
{
	unsigned int intc;
	
	if( ! addr || ! hookfunc ) return;
	
	/* セマフォで排他処理 */
	intc = pspSdkDisableInterrupts();
	
	/* 関数を変更 */
	*addr = hookfunc;
	
	/* 変更部分を適用 */
	sceKernelDcacheWritebackInvalidateRange( addr, sizeof( addr ) );
	sceKernelIcacheInvalidateRange( addr, sizeof( addr ) );
	
	pspSdkEnableInterrupts( intc );
}
