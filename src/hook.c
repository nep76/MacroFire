/*
	APIフック
	(内容をほとんど把握していないので、大部分をPSPLinkのソースから流用)
*/

#include "hook.h"

struct SyscallHeader
{
	void *unk;
	unsigned int basenum;
	unsigned int topnum;
	unsigned int size;
};

static void *hookFindSyscallAddr(unsigned int addr);
static SceUID hookFindModuleUidByName( const char *modname );
static SceLibraryEntryTable *hookFindLibrary( SceUID uid, const char *library );
static void *hookFindExportAddrByNid( SceUID uid, const char *library, unsigned int nid );
static unsigned int hookFindExportByNid( SceUID uid, const char *library, unsigned int nid );
static void *hookApiAddr( SysApi *syscall, unsigned int *addr, void *func );

static void *hookFindSyscallAddr(unsigned int addr)
{
	/* SYSCALL例外発生時に呼ばれる例外ハンドラの位置を特定する？ */
	
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
		if(syscalls[i] == addr)
		{
			return &syscalls[i];
		}
	}
	
	return NULL;
}

/* モジュール名からUIDを取得 */
static SceUID hookFindModuleUidByName( const char *modname )
{
	SceModule *modinfo;
	
	if( ! modname ) return 0;
	
	modinfo = sceKernelFindModuleByName( modname );
	if( ! modinfo ) return 0;
	
	return modinfo->modid;
}

/* モジュールがエクスポートしているライブラリエントリを返す */
static SceLibraryEntryTable *hookFindLibrary( SceUID uid, const char *library )
{
	SceModule *modinfo;
	SceLibraryEntryTable *entry;
	
	/* モジュール情報を取得 */
	modinfo = sceKernelFindModuleByUID( uid );
	
	if( modinfo ){
		int i = 0;
		
		/* ent_topメンバがライブラリエントリの先頭を示す */
		entry = (SceLibraryEntryTable *)modinfo->ent_top;
		
		/* 順番にライブラリエントリの名前がlibraryに一致するか調べる */
		while( i < modinfo->ent_size ){
			if(
				( entry->libname && ( strcmp( entry->libname, library ) == 0 ) ) ||
				( ! entry->libname && ! library )
			){
				return entry;
			}
			entry++;
		}
	}
	
	return NULL;
}

static void *hookFindExportAddrByNid( SceUID uid, const char *library, unsigned int nid )
{
	SceLibraryEntryTable *entry;
	unsigned int *addr = 0;
	
	/* 目的のライブラリエントリを取得 */
	entry = hookFindLibrary( uid, library );
	if( entry ){
		/* エクスポートしている関数があれば調べる */
		if( entry->stubcount > 0 ){
			
			/* NIDテーブルを取得し、NIDと一致する要素を探す */
			unsigned int *nidtable = entry->entrytable;
			
			int i;
			for( i = 0; i < entry->stubcount; i++ ){
				/*
					一致する要素が見つかったらポインタを返すが、全NIDリスト分ずらして返す。
					どうやらentrytableには、先頭からまずNIDが並べられ、
					その後ろに実際のAPIとして呼ばれる関数ポインタ(を導く何かのアドレス？)が保存されているっぽい。
				*/
				if( nidtable[i] == nid ) return &nidtable[i + entry->stubcount + entry->vstubcount];
			}
		}
	}
	
	return addr;
}

static unsigned int hookFindExportByNid( SceUID uid, const char *library, unsigned int nid )
{
	unsigned int *addr;
	
	addr = hookFindExportAddrByNid( uid, library, nid );
	if( ! addr ) return 0;
	
	return *addr;
}

static void *hookApiAddr( SysApi *syscall, unsigned int *addr, void *func )
{
	int intr;
	
	if( ! addr ) return NULL;
	
	/* Interruptってなんじゃらほい */
	intr = pspSdkDisableInterrupts();
	
	/* 既存のAPI情報を保存 */
	if( syscall ){
		syscall->addr = (void *)addr;
		syscall->func = (void *)(*addr);
	}
	
	/* APIとして呼ばれる関数ポインタを変更! */
	*addr = (unsigned int)func;
	
	/* 以下2行はAPIとして呼ばれる関数ポインタを確実に書き換えるためのキャッシュ処理だと思われる。*/
	
	/* CPU内のデータキャッシュ(Dcache)から、指定した範囲のキャッシュをメモリに書き戻して無効にする? */
	sceKernelDcacheWritebackInvalidateRange( addr, sizeof( addr ) );
	/* CPU内のインストラクションキャッシュ(Icache)から、指定した範囲を無効にする? */
	sceKernelIcacheInvalidateRange( addr, sizeof( addr ) );
	
	pspSdkEnableInterrupts( intr );
	
	return addr;
}

unsigned int hookApiByNid( SysApi *syscall, const char *modname, const char *library, unsigned int nid, void *func )
{
	unsigned int addr;
	SceUID uid;
	
	/* モジュール名からモジュールUIDを取得 */
	uid = hookFindModuleUidByName( modname );
	
	/* NIDテーブルを参照して目的のAPIを検索 */
	addr = hookFindExportByNid( uid, library, nid );
	
	if( addr ){
		/*
			MIPSのSYSCALL例外発生時に呼ばれる例外ハンドラの位置を特定する？
		*/
		if( ! hookApiAddr( syscall, hookFindSyscallAddr( addr ), func ) ){
			addr = 0;
		}
	}
	
	return addr;
}

void *hookRestoreAddr( SysApi *syscall )
{
	void *addr = 0;
	
	if( syscall->addr )
		addr = hookApiAddr( NULL, (void *)syscall->addr, (void *)syscall->func );
	
	if( addr ){
		syscall->addr = 0;
		syscall->func = 0;
	}
	
	return addr;
}
