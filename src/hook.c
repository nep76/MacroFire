/*
	hook.c
*/

#include "hook.h"

struct SyscallHeader
{
	void *unk;
	unsigned int basenum;
	unsigned int topnum;
	unsigned int size;
};

void *hookFindSyscallAddr( void *addr )
{
	/*
		この関数のコードはPSPLinkのソースから丸ごとコピーしました。
		
		SYSCALL例外発生時に呼ばれる例外ハンドラの位置を特定している?
		アセンブリコードが全然わからないのでコード内容は不明。
		CFC0命令がまずなんだかわからないし、$12レジスタに一体何が書き込まれているのか？
	*/
	
	struct SyscallHeader *head;
	unsigned int *syscalls;
	void **ptr;
	int size;
	int i;
	
	asm( "cfc0 %0, $12\n" : "=r"(ptr) );
	
	if(!ptr)
	{
		return NULL;
	}
	
	head = (struct SyscallHeader *) *ptr;
	syscalls = (unsigned int*) (*ptr + 0x10);
	size = (head->size - 0x10) / sizeof(unsigned int);
	
	for(i = 0; i < size; i++)
	{
		if(syscalls[i] == (unsigned int)addr)
		{
			return &syscalls[i];
		}
	}
	
	return NULL;
}

SceLibraryEntryTable *hookGetLibraryEntry( SceModule *modinfo, const char *libname )
{
	int libent_count = 0;
	SceLibraryEntryTable *libent;
	
	if( ! modinfo ) return NULL;
	
	libent = (SceLibraryEntryTable *)modinfo->ent_top;
	
	while( libent_count < modinfo->ent_size ){
		if( strcmp( libent->libname, libname ) == 0 ){
			return libent;
		}
		libent++;
		libent_count++;
	}
	
	return NULL;
}

void *hookGetExportedAddrPointer( SceLibraryEntryTable *libent, unsigned int nid )
{
	unsigned int *nid_table;
	int entry_count = 0;
	
	/* エクスポートしている関数がなければ戻る */
	if( libent->stubcount <= 0 ) return NULL;
	
	nid_table = libent->entrytable;
	
	while( entry_count < libent->stubcount ){
		if( nid_table[entry_count] == nid ){
			return (void *)nid_table[libent->stubcount + libent->vstubcount + entry_count];
		}
		entry_count++;
	}
	
	return NULL;
}

void *hookFindExportedAddr( const char *modname, const char *libname, unsigned int nid )
{
	SceModule *modinfo;
	SceLibraryEntryTable *libent;
	void *addr;
	
	/* モジュール情報を取得 */
	modinfo = sceKernelFindModuleByName( modname );
	if( ! modinfo ) return NULL;
	
	/* ライブラリ情報を取得 */
	libent = hookGetLibraryEntry( modinfo, libname );
	if( ! libent ) return NULL;
	
	/* NIDからAPIのエントリポイントが保存されているアドレスを検出 */
	addr = hookGetExportedAddrPointer( libent, nid );
	
	return addr;
}

void *hookUpdateExportedAddr( void **addr, void *entrypoint )
{
	void *prev_entrypoint;
	
	if( ! addr || ! entrypoint ) return NULL;
	
	/* すでにセットされているAPIのエントリポイントをコピー */
	prev_entrypoint = *addr;
	
	/* フック関数となる新しいエントリポイントをセット */
	*addr = entrypoint;
	
	/* CPU内のデータキャッシュ(Dcache)から、指定した範囲のキャッシュをメモリに書き戻して無効にする? */
	sceKernelDcacheWritebackInvalidateRange( addr, sizeof( addr ) );
	
	/* CPU内のインストラクションキャッシュ(Icache)から、指定した範囲を無効にする? */
	sceKernelIcacheInvalidateRange( addr, sizeof( addr ) );
	
	/* 今までセットされていた古いAPIのエントリポイントを返す */
	return prev_entrypoint;
}
