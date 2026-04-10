/* dirhChdir.c */

#include "dirh_types.h"
#include "util/strutil.h"

static int dirh_make_entries( const char *dirpath, struct dirh_entry *entry, struct dirh_thread_dopen_params **thdopen, unsigned int timeout, bool alloc_high );
static int dirh_dopen_wrapper( SceSize args, void *argp );
static bool dirh_thread_dopen_ready( struct dirh_thread_dopen_params **thdopen );
static bool dirh_thread_dopen( struct dirh_thread_dopen_params *thdopen, const char *dirpath, unsigned int timeout );

int dirhChdir( DirhUID uid, const char *dirpath, unsigned int timeout )
{
	struct dirh_params *params = (struct dirh_params *)uid;
	
	/* パスを展開 */
	//if( ! pathexpandFromCwd( dirpath, params->cwd.path, params->cwd.length ) ) return CG_ERROR_NO_FILE_ENTRY;
	strutilCopy( params->cwd.path, dirpath, params->cwd.length );
	makepath( "ms0:/", params->cwd.path, params->cwd.length );
	
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
	
	if( params->entry.list ) __dirh_clear_entries( &(params->entry) );
	
	return dirh_make_entries( params->cwd.path, &(params->entry), ( ( params->options & DIRH_O_DOPEN_WITH_THREAD ) ? &(params->thdopen) : NULL ), timeout, params->options & DIRH_O_ALLOC_HIGH ? true : false );
}

void __dirh_clear_entries( struct dirh_entry *entry )
{
	memoryFree( entry->list );
	entry->list  = NULL;
	entry->count = 0;
	entry->pos   = 0;
}

int __dirh_parse_dirent( const char *dirpath, struct dirh_entry *entry )
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

bool __dirh_wait_for_dopen_thread( struct dirh_thread_dopen_params *thdopen, enum PspThreadStatus stat )
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

static int dirh_make_entries( const char *dirpath, struct dirh_entry *entry, struct dirh_thread_dopen_params **thdopen, unsigned int timeout, bool alloc_high )
{
	if( thdopen ){
		if( ! *thdopen ){
			*thdopen = (struct dirh_thread_dopen_params *)memoryExalloc( "DirhDopen", MEMORY_USER, 0, sizeof( struct dirh_thread_dopen_params ), alloc_high ? PSP_SMEM_High : PSP_SMEM_Low, NULL );
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
		return __dirh_parse_dirent( dirpath, entry );
	}
}

static int dirh_dopen_wrapper( SceSize args, void *argp )
{
	struct dirh_thread_dopen_params **thdopen = (*(struct dirh_thread_dopen_params ***)argp);
	
	for( ;; ){
		sceKernelSleepThread();
		if( ! (*thdopen)->path ) break;
		__dirh_parse_dirent( (*thdopen)->path, (*thdopen)->entry );
		(*thdopen)->completed = true;
	}
	
	memoryFree( *thdopen );
	*thdopen = NULL;
	
	return sceKernelExitDeleteThread( 0 );
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
	
	if( ! __dirh_wait_for_dopen_thread( thdopen, PSP_THREAD_WAITING ) ) return false;
	
	thdopen->path = dirpath;
	thdopen->completed = false;
	
	ret = sceKernelWakeupThread( thdopen->selfThreadId );
	
	sceKernelLibcTime( &start_time );
	
	/* Dopenに成功するかタイムアウトするまで待つ */
	for( ; ! thdopen->completed && ( sceKernelLibcTime( NULL ) - start_time < timeout ); sceKernelDelayThread( 10000 ) );
	
	return thdopen->completed ? true : false;
}
