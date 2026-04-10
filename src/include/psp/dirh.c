/*=========================================================

	dirh.c
	
	ディレクトリハンドラ。

=========================================================*/
#include "dirh.h"

/*=========================================================
	ローカル型宣言
=========================================================*/

struct dirh_cwd {
	char  *path;
	size_t length;
};

struct dirh_entry {
	DirhFileInfo *list;
	unsigned int count;
	int          pos;
};

struct dirh_thread_dopen_params {
	SceUID     selfThreadId;
	const char *path;
	bool       completed;
	struct dirh_entry *entry;
};

struct dirh_params {
	struct dirh_cwd cwd;
	struct dirh_entry entry;
	unsigned int options;
};

/*=========================================================
	ローカル関数
=========================================================*/
static int dirh_make_entries( const char *dirpath, struct dirh_entry *entry, struct dirh_thread_dopen_params **thdopen, unsigned int timeout, bool alloc_high );
static void dirh_clear_entries( struct dirh_entry *entry );
static int dirh_dopen_wrapper( SceSize args, void *argp );
static bool dirh_wait_for_dopen_thread( struct dirh_thread_dopen_params *thdopen, enum PspThreadStatus stat );
static bool dirh_thread_dopen_ready( struct dirh_thread_dopen_params **thdopen );
static bool dirh_thread_dopen( struct dirh_thread_dopen_params *thdopen, const char *dirpath, unsigned int timeout );
static void dirh_thread_dopen_exit( struct dirh_thread_dopen_params *thdopen );
static int dirh_parse_dirent( const char *dirpath, struct dirh_entry *entry );
static int dirh_default_sort( const void *a, const void *b );

/*=========================================================
	ローカル変数
=========================================================*/
static struct dirh_thread_dopen_params *st_thdopen;

/*=========================================================
	関数
=========================================================*/
DirhUID dirhNew( size_t pathmax, unsigned int options )
{
	struct dirh_params *params = (struct dirh_params *)memoryAllocEx( "DirhParams", MEMORY_USER, 0, sizeof( struct dirh_params ) + pathmax, options & DIRH_O_ALLOC_HIGH ? PSP_SMEM_High : PSP_SMEM_Low, NULL );
	if( ! params ) return 0;
	
	/* 初期化 */
	params->cwd.path    = (char *)( (uintptr_t)params + sizeof( struct dirh_params ) );
	params->cwd.length  = pathmax;
	params->entry.list  = NULL;
	params->entry.count = 0;
	params->entry.pos   = 0;
	params->options     = options;
	
	params->cwd.path[0] = '\0';
	
	return (DirhUID)params;
}

int dirhChdir( DirhUID uid, const char *dirpath, unsigned int timeout )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	/* パスを展開 */
	if( ! pathexpandFromCwd( dirpath, params->cwd.path, params->cwd.length ) ) return CG_ERROR_NO_FILE_ENTRY;
	
	if( params->cwd.path[strlen( params->cwd.path ) - 1] != ':' ){
		/* 末尾がコロンではない場合はドライブ指定ではないのでパスを解析 */
		SceIoStat stat;
		int ret;
		
		memset( &stat, 0, sizeof( SceIoStat ) );
		
		/* 指定されたパスの情報を取得 */
		if( ( ret = sceIoGetstat( params->cwd.path, &stat ) ) < 0 ){
			return CG_ERROR_FAILED_TO_GETSTAT;
		}
		
		if( ! FIO_S_ISDIR( stat.st_mode ) ) return CG_ERROR_PATH_IS_NOT_DIR;
	}
	
	if( params->entry.list ) dirh_clear_entries( &(params->entry) );
	
	return dirh_make_entries( params->cwd.path, &(params->entry), ( ( params->options & DIRH_O_DOPEN_WITH_THREAD ) ? &st_thdopen : NULL ), timeout, params->options & DIRH_O_ALLOC_HIGH ? true : false );
}

void dirhDestroy( DirhUID uid )
{
	if( ! uid ) return;
	
	if( ((struct dirh_params *)uid)->entry.list ) dirh_clear_entries( &(((struct dirh_params *)uid)->entry) );
	
	if( st_thdopen ){
		dirh_wait_for_dopen_thread( st_thdopen, PSP_THREAD_WAITING );
		dirh_thread_dopen_exit( st_thdopen );
	}
	
	memoryFree( (void *)uid );
}

DirhFileInfo *dirhRead( DirhUID uid )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	if( params->entry.count <= params->entry.pos ) return NULL;
	
	return &(params->entry.list[params->entry.pos++]);
}

int dirhTell( DirhUID uid )
{
	return ((struct dirh_params *)uid)->entry.pos;
}

char *dirhGetCwd( DirhUID uid )
{
	return ((struct dirh_params *)uid)->cwd.path;
}

void dirhSort( DirhUID uid, int ( *compare )( const void*, const void* ) )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	qsort( params->entry.list, params->entry.count, sizeof( DirhFileInfo ), compare ? compare : dirh_default_sort );
}

void dirhSeek( DirhUID uid, DirhWhence whence, int offset )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	switch( whence ){
		case DIRH_SEEK_SET: params->entry.pos = 0; break;
		case DIRH_SEEK_END: params->entry.pos = params->entry.count; break;
		case DIRH_SEEK_CUR: break;
	}
	
	params->entry.pos += offset;
	
	if( params->entry.pos < 0 ){
		params->entry.pos = 0;
	} else if( params->entry.pos > params->entry.count ){
		params->entry.pos = params->entry.count;
	}
}

static int dirh_make_entries( const char *dirpath, struct dirh_entry *entry, struct dirh_thread_dopen_params **thdopen, unsigned int timeout, bool alloc_high )
{
	if( thdopen ){
		if( ! *thdopen ){
			*thdopen = (struct dirh_thread_dopen_params *)memoryAllocEx( "DirhDopen", MEMORY_USER, 0, sizeof( struct dirh_thread_dopen_params ), alloc_high ? PSP_SMEM_High : PSP_SMEM_Low, NULL );
			if( *thdopen ){
				(*thdopen)->selfThreadId = 0;
				(*thdopen)->path         = NULL;
				(*thdopen)->completed    = false;
				(*thdopen)->entry        = entry;
			} else{
				return CG_ERROR_NOT_ENOUGH_MEMORY;
			}
		}
		if( *thdopen ){
			bool ret;
			
			if( ! dirh_thread_dopen_ready( thdopen ) ) return CG_ERROR_FAILED_TO_DOPEN;
			ret = dirh_thread_dopen( *thdopen, dirpath, timeout );
			
			return ret ? 0: CG_ERROR_FAILED_TO_DOPEN;
		} else{
			return CG_ERROR_FAILED_TO_DOPEN;
		}
	} else{
		return dirh_parse_dirent( dirpath, entry );
	}
}

static int dirh_parse_dirent( const char *dirpath, struct dirh_entry *entry )
{
	SceUID fd;
	SceIoDirent dirent;
	size_t require_memsize;
	int ret;
	
	char *strings;
	unsigned int i, len;
	
	fd = sceIoDopen( dirpath );
	if( ! fd ) return CG_ERROR_FAILED_TO_DOPEN;
	
	memset( &dirent, 0, sizeof( SceIoDirent ) );
	
	require_memsize = 0;
	while( ( ret = sceIoDread( fd, &dirent ) ) > 0 ){
		if( ( dirent.d_stat.st_attr & ( FIO_SO_IFREG | FIO_SO_IFDIR ) ) && strcmp( dirent.d_name, "." ) != 0 ){
			require_memsize += strlen( dirent.d_name ) + 1;
			entry->count++;
		}
	}
	sceIoDclose( fd );
	
	fd = sceIoDopen( dirpath );
	if( ! fd ) return CG_ERROR_FAILED_TO_DOPEN;
	
	entry->list = (DirhFileInfo *)memoryAlloc( sizeof( DirhFileInfo ) * entry->count + require_memsize );
	if( ! entry->list ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	strings     = (char *)((uintptr_t)(entry->list) + sizeof( DirhFileInfo ) * entry->count);
	
	memset( &dirent, 0, sizeof( SceIoDirent ) );
	
	/* ディレクトリリストを作成 */
	i = 0;
	while( ( ret = sceIoDread( fd, &dirent ) ) > 0 ){
		if( strcmp( dirent.d_name, "." ) != 0 && i < entry->count ){
			if( dirent.d_stat.st_attr & FIO_SO_IFREG ){
				entry->list[i].type = DIRH_FILE;
			} else if( dirent.d_stat.st_attr & FIO_SO_IFDIR ){
				entry->list[i].type = DIRH_DIR;
			} else{
				continue;
			}
			
			entry->list[i].name = strings;
			
			len = strlen( dirent.d_name ) + 1;
			strcpy( strings, dirent.d_name );
			strings += len;
			
			i++;
		}
	}
	
	sceIoDclose( fd );
	
	return 0;
}

static void dirh_clear_entries( struct dirh_entry *entry )
{
	memoryFree( entry->list );
	entry->list  = NULL;
	entry->count = 0;
	entry->pos   = 0;
}

static int dirh_dopen_wrapper( SceSize args, void *argp )
{
	struct dirh_thread_dopen_params **thdopen = (*(struct dirh_thread_dopen_params ***)argp);
	
	for( ;; ){
		sceKernelSleepThread();
		if( ! (*thdopen)->path ) break;
		dirh_parse_dirent( (*thdopen)->path, (*thdopen)->entry );
		(*thdopen)->completed = true;
	}
	
	memoryFree( *thdopen );
	*thdopen = NULL;
	
	return sceKernelExitDeleteThread( 0 );
}

static bool dirh_wait_for_dopen_thread( struct dirh_thread_dopen_params *thdopen, enum PspThreadStatus stat )
{
	SceKernelThreadInfo thstat;
	thstat.size = sizeof( SceKernelThreadInfo );
	
	/* Dopenスレッドを待つ */
	do{
		if( sceKernelReferThreadStatus( thdopen->selfThreadId, &thstat ) != 0 ) return false;
		sceKernelDelayThread( 1000 );
	} while( thstat.status != PSP_THREAD_WAITING );
	
	return true;
}

static bool dirh_thread_dopen_ready( struct dirh_thread_dopen_params **thdopen )
{
	if( ! (*thdopen)->selfThreadId ){
		(*thdopen)->selfThreadId = sceKernelCreateThread( "dirhDopenWrapper", dirh_dopen_wrapper, 32, 0x800, PSP_THREAD_ATTR_NO_FILLSTACK, NULL );
		if( (*thdopen)->selfThreadId < 0 ) return false;
		if( sceKernelStartThread( (*thdopen)->selfThreadId, sizeof( struct dirh_thread_dopen_params * ), &thdopen ) < 0 ) return false;
	}
	return true;
}

static bool dirh_thread_dopen( struct dirh_thread_dopen_params *thdopen, const char *dirpath, unsigned int timeout )
{
	time_t start_time;
	int ret;
	
	if( ! dirh_wait_for_dopen_thread( thdopen, PSP_THREAD_WAITING ) ) return false;
	
	thdopen->path = dirpath;
	thdopen->completed = false;
	
	ret = sceKernelWakeupThread( thdopen->selfThreadId );
	
	sceKernelLibcTime( &start_time );
	
	/* Dopenに成功するかタイムアウトするまで待つ */
	for( ; ! thdopen->completed && ( sceKernelLibcTime( NULL ) - start_time < timeout ); sceKernelDelayThread( 10000 ) );
	
	return thdopen->completed ? true : false;
}

static void dirh_thread_dopen_exit( struct dirh_thread_dopen_params *thdopen )
{
	if( thdopen->selfThreadId ){
		thdopen->path = NULL;
		sceKernelWakeupThread( thdopen->selfThreadId );
	}
}

static int dirh_default_sort( const void *a, const void *b )
{
	DirhFileInfo *file_a = (DirhFileInfo *)a;
	DirhFileInfo *file_b = (DirhFileInfo *)b;
	
	if( file_a->type == file_b->type ){
		return strcasecmp( file_a->name, file_b->name );
	} else if( file_a->type == DIRH_DIR ){
		return -1;
	} else{
		return 1;
	}
}
