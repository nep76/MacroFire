/*
	dirh.c
*/

#include "dirh.h"

static enum dirh_childthread_stat dirh_childthread_getstat( struct dirh_dopen_params *params );
static void dirh_childthread_chstat( struct dirh_dopen_params *params, enum dirh_childthread_stat stat );
static int dirh_childthread_dopen( SceSize args, void *argp );
static SceUID dirh_get_dirent_thread( const char *dirpath, struct dirh_select_filename *sf );
//static int dirh_get_dirent( const char *dirpath, struct dirh_select_filename *sf );
static void dirh_destroy_dirent( struct dirh_dirents *entry );
static int dirh_error( struct dirh_select_filename *sf, int lerror, int serror );

static bool st_thread_worked = false;

static enum dirh_childthread_stat dirh_childthread_getstat( struct dirh_dopen_params *params )
{
	enum dirh_childthread_stat stat;
	
	sceKernelWaitSema( params->lock, 1, 0 );
	stat = params->status;
	sceKernelSignalSema( params->lock, 1 );
	
	return stat;
}

static void dirh_childthread_chstat( struct dirh_dopen_params *params, enum dirh_childthread_stat stat )
{
	sceKernelWaitSema( params->lock, 1, 0 );
	params->status = stat;
	sceKernelSignalSema( params->lock, 1 );
}

static int dirh_childthread_dopen( SceSize args, void *argp )
{
	struct dirh_dopen_params *params = (*(struct dirh_dopen_params **)argp);
	SceIoDirent dirent;
	int ret;
	int file_index, dir_index;
	
	dirh_childthread_chstat( params, DIRH_CHILDTHREAD_INIT );
	
	params->lock = sceKernelCreateSema( "dopen_wrapper_sema", 0, 1, 1, 0 );
	
	dirh_childthread_chstat( params, DIRH_CHILDTHREAD_READY );
	
	params->fd = sceIoDopen( params->path );
	if( dirh_childthread_getstat( params ) == DIRH_CHILDTHREAD_ABORT ){
		goto ABORT;
	} else if( params->fd < 0 ){
		dirh_error( params->sf, CG_ERROR_FAILED_TO_DOPEN, params->fd );
		goto ABORT;
	}
	
	memset( &dirent, 0, sizeof( SceIoDirent ) );
	
	dirh_childthread_chstat( params, DIRH_CHILDTHREAD_COUNT );
	
	while( ( ret = sceIoDread( params->fd, &dirent ) ) > 0 ){
		if( dirent.d_stat.st_attr & FIO_SO_IFDIR ){
			if( strcmp( dirent.d_name, "." ) == 0 ) continue;
			params->sf->entry.dirCount++;
		} else if( dirent.d_stat.st_attr & FIO_SO_IFREG ){
			params->sf->entry.fileCount++;
		}
	}
	
	sceIoDclose( params->fd );
	
	/* sceIoDread()がエラーを返していれば終了 */
	if( dirh_childthread_getstat( params ) == DIRH_CHILDTHREAD_ABORT ){
		goto ABORT;
	} else if( ret < 0 ){
		dirh_error( params->sf, CG_ERROR_FAILED_TO_DOPEN, ret );
		goto ABORT;
	}
	
	/* 読み込む準備 */
	params->sf->entry.dirList = (char **)memsceMalloc( sizeof( char* ) * params->sf->entry.dirCount );
	if( ! params->sf->entry.dirList ) goto ABORT;
	params->sf->entry.fileList = (char **)memsceMalloc( sizeof( char* ) * params->sf->entry.fileCount );
	if( ! params->sf->entry.fileList ) goto ABORT;
	
	/* 読み込む */
	params->fd = sceIoDopen( params->path );
	if( dirh_childthread_getstat( params ) == DIRH_CHILDTHREAD_ABORT ){
		goto ABORT;
	} else if( params->fd < 0 ){
		dirh_error( params->sf, CG_ERROR_FAILED_TO_DOPEN, params->fd );
		goto ABORT;
	}
	
	memset( &dirent, 0, sizeof( SceIoDirent ) );
	
	dirh_childthread_chstat( params, DIRH_CHILDTHREAD_READ );
	
	for( file_index = 0, dir_index = 0; sceIoDread( params->fd, &dirent ) > 0;  ){
		if( FIO_S_ISDIR( dirent.d_stat.st_mode ) ){
			if( strcmp( dirent.d_name, "." ) == 0 ) continue;
			
			/*
				NULLとディレクトリを表す"/"のために2バイト多く確保。
				DIRH_OPT_ADD_DIR_SLASHで"/"を付加する設定になっていなくても関係なしに2バイト多く確保。
			*/
			params->sf->entry.dirList[dir_index] = (char *)memsceMalloc( sizeof( char ) * ( strlen( dirent.d_name ) + 2 ) );
			if( ! params->sf->entry.dirList[dir_index] ) break;
			
			strcpy( params->sf->entry.dirList[dir_index], dirent.d_name );
			if( params->sf->options & DIRH_OPT_ADD_DIR_SLASH ) strcat( params->sf->entry.dirList[dir_index], "/" );
			
			dir_index++;
		} else if( FIO_S_ISREG( dirent.d_stat.st_mode ) ){
			params->sf->entry.fileList[file_index] = (char *)memsceMalloc( sizeof( char ) * ( strlen( dirent.d_name ) + 1 ) );
			if( ! params->sf->entry.fileList[file_index] ) break;
			
			strcpy( params->sf->entry.fileList[file_index], dirent.d_name );
			file_index++;
		}
	}
	
	if( dir_index < params->sf->entry.dirCount || file_index < params->sf->entry.fileCount ){
		// エントリを漏らさずに読み込むことができなかったとき
	}
	
	sceIoDclose( params->fd );
	
	goto QUIT;
	
	ABORT:
		memsceFree( params->sf->entry.fileList );
		memsceFree( params->sf->entry.dirList );
		if( params->fd >= 0 ) sceIoDclose( params->fd );
		goto QUIT;
		
	QUIT:
		sceKernelDeleteSema( params->lock );
		memsceFree( params );
		st_thread_worked = false;
		return sceKernelExitDeleteThread( 0 );
}

static int dirh_get_dirent_thread( const char *dirpath, struct dirh_select_filename *sf )
{
	struct dirh_dopen_params *params;
	SceUID thid;
	time_t start_time;
	
	if( ! dirpath || st_thread_worked ) return CG_ERROR_FAILED_TO_DOPEN;
	
	/* 
		子スレッド内でsceIoDopenからディレクトリリストを読み込む作業を行うが、
		sceIoDopenは、他のスレッドがすでにsceIoDopen中だと(?)自分が開けるまで延々待ち続ける。
		
		通常あまり問題はないが、プラグインなどでスレッドを停止した場合、
		この待機により操作不能になる。
		
		そのため、子スレッドで開くが、スレッドをリジュームした際、
		子スレッド内で行ったsceIoDopenは待機を続けているため、外から子スレッドを殺すと
		sceIoDopenされっぱなしになりゲームなどが逆に止まる。
		
		そのため、この関数からは読み取りを行えと子スレッドに指示し、
		規定時間以内に子スレッド実行フラグ(st_thread_worked)が消えない場合は、
		子スレッドに中断フラグ(DIRH_CHILDTHREAD_ABORT)を設定し、抜ける。
		
		このフラグは、子スレッドがsceIoDopenを成功させたあと(他のスレッドがリジュームされた時)、
		判定されることになるので、これが終わるまでparamsを解放する訳にはいかない。
		
		従って、このparama変数に割り当てたメモリは、子スレッドから解放される。
	*/
	params = memsceCalloc( 1, sizeof( struct dirh_dopen_params ) + strlen( dirpath ) + 1 );
	if( ! params ) return CG_ERROR_FAILED_TO_DOPEN;
	
	params->fd   = -1;
	params->path = (char *)( (unsigned int)params + sizeof( struct dirh_dopen_params ) );
	strcpy( params->path, dirpath );
	params->sf   = sf;
	
	thid = sceKernelCreateThread( "dopen_wrapper", dirh_childthread_dopen, 32, 2048, PSP_THREAD_ATTR_NO_FILLSTACK, NULL );
	if( thid < 0 || sceKernelStartThread( thid, sizeof( struct dir_dopen_params ** ), &params ) < 0 ) return CG_ERROR_FAILED_TO_DOPEN;
	
	st_thread_worked = true;
	
	for( ; dirh_childthread_getstat( params ) < DIRH_CHILDTHREAD_READY; sceKernelDelayThread( 50000 ) );
	
	sceKernelLibcTime( &start_time );
	
	/* sceIoDopenが固まっていないかをチェック */
	for( ; ; sceKernelDelayThread( 50000 ) ){
		if( ! st_thread_worked ){
			break;
		} else if( sceKernelLibcTime( NULL ) - start_time >= 4 ){
			dirh_childthread_chstat( params, DIRH_CHILDTHREAD_ABORT );
			return CG_ERROR_FAILED_TO_DOPEN;
		}
	}
	
	return 0;
}

static int dirh_error( struct dirh_select_filename *sf, int lerror, int serror )
{
	sf->lError = lerror;
	sf->sError = serror;
	return sf->lError;
}

/* オプションでスレッド使用モードと非使用モードを切り替えられた方がよい?
static int dirh_get_dirent( const char *dirpath, struct dirh_select_filename *sf )
{
	SceUID dfd;
	SceIoDirent dir;
	int ret, cnt_file = 0, cnt_dir = 0;
	int fi = 0, di = 0;
	
	// エントリ数を数える
	dfd = sceIoDopen( dirpath );
	if( dfd < 0 ) return dirh_error( sf, CG_ERROR_FAILED_TO_DOPEN, dfd );
	
	memset( &dir, 0, sizeof( SceIoDirent ) );
	
	while( ( ret = sceIoDread( dfd, &dir ) ) > 0 ){
		if( dir.d_stat.st_attr & FIO_SO_IFDIR ){
			if( strcmp( dir.d_name, "." ) == 0 ) continue;
			cnt_dir++;
		} else if( dir.d_stat.st_attr & FIO_SO_IFREG ){
			cnt_file++;
		}
	}
	
	sceIoDclose( dfd );
	
	// sceIoDread()がエラーを返していればfalse
	if( ret < 0 ) return dirh_error( sf, CG_ERROR_FAILED_TO_DREAD, ret );
	
	// 読み込む準備
	sf->entry.dirList = (char **)memsceMalloc( sizeof( char* ) * cnt_dir );
	if( ! sf->entry.dirList ) goto DESTROY;
	sf->entry.fileList = (char **)memsceMalloc( sizeof( char* ) * cnt_file );
	if( ! sf->entry.fileList ) goto DESTROY;
	sf->entry.dirCount  = cnt_dir;
	sf->entry.fileCount = cnt_file;
	
	// 読み込む
	dfd = sceIoDopen( dirpath );
	if( dfd < 0 ){
		dirh_error( sf, CG_ERROR_FAILED_TO_DOPEN, dfd );
		goto DESTROY;
	}
	
	memset( &dir, 0, sizeof( SceIoDirent ) );
	
	for( fi = 0, di = 0; sceIoDread( dfd, &dir ) > 0;  ){
		if( FIO_S_ISDIR( dir.d_stat.st_mode ) ){
			if( strcmp( dir.d_name, "." ) == 0 ) continue;
			
			// NULLとディレクトリを表す"/"のために2バイト多く確保。
			// DIRH_OPT_ADD_DIR_SLASHで"/"を付加する設定になっていなくても関係なしに2バイト多く確保。
			sf->entry.dirList[di] = (char *)memsceMalloc( sizeof( char ) * ( strlen( dir.d_name ) + 2 ) );
			if( ! sf->entry.dirList[di] ) break;
			
			strcpy( sf->entry.dirList[di], dir.d_name );
			if( sf->options & DIRH_OPT_ADD_DIR_SLASH ) strcat( sf->entry.dirList[di], "/" );
			
			di++;
		} else if( FIO_S_ISREG( dir.d_stat.st_mode ) ){
			sf->entry.fileList[fi] = (char *)memsceMalloc( sizeof( char ) * ( strlen( dir.d_name ) + 1 ) );
			if( ! sf->entry.fileList[fi] ) break;
			
			strcpy( sf->entry.fileList[fi], dir.d_name );
			fi++;
		}
	}
	
	if( di < sf->entry.dirCount || fi < sf->entry.fileCount ){
		// エントリを漏らさずに読み込むことができなかったとき
	}
	
	sceIoDclose( dfd );
	
	return 0;
	
	DESTROY:
		memsceFree( sf->entry.fileList );
		memsceFree( sf->entry.dirList );
		if( dfd >= 0 ) sceIoDclose( dfd );
		return sf->lError;
}
*/

static void dirh_destroy_dirent( struct dirh_dirents *entry )
{
	int i;
	if( ! entry ) return;
	
	if( entry->dirList ){
		if( entry->dirCount ){
			for( i = 0; i < entry->dirCount; i++ ) memsceFree( entry->dirList[i] );
		}
		memsceFree( entry->dirList );
	}
	
	if( entry->fileList ){
		if( entry->fileCount ){
			for( i = 0; i < entry->fileCount; i++ ) memsceFree( entry->fileList[i] );
		}
		memsceFree( entry->fileList );
	}
	
	entry->pos        = 0;
	entry->dirList    = NULL;
	entry->dirCount   = 0;
	entry->fileList   = NULL;
	entry->fileCount  = 0;
}

DirhUID dirhNew( const char *dirpath, unsigned int options )
{
	struct dirh_select_filename *sf = (struct dirh_select_filename *)memsceMalloc( sizeof( struct dirh_select_filename ) );
	int ret;
	
	if( ! sf ) return 0;
	
	sf->curDirFullpath[0] = '\0';
	sf->options           = options;
	
	sf->entry.pos       = 0;
	sf->entry.dirList   = NULL;
	sf->entry.dirCount  = 0;
	sf->entry.fileList  = NULL;
	sf->entry.fileCount = 0;
	
	ret = dirhChdir( (DirhUID)sf, dirpath );
	
	if( ret < 0 ){
		memsceFree( sf );
		return ret;
	} else{
		return (DirhUID)sf;
	}
}

void dirhDestroy( DirhUID uid )
{
	dirh_destroy_dirent( &(((struct dirh_select_filename *)uid)->entry) );
	memsceFree( (struct dirh_select_filename *)uid );
}

int dirhChdir( DirhUID uid, const char *dirpath )
{
	int ret;
	SceIoStat stat;
	struct dirh_select_filename *sf = (struct dirh_select_filename *)uid;
	
	dirh_error( sf, CG_ERROR_OK, 0 );
	
	if( ! pathexpandFromCwd( dirpath, sf->curDirFullpath, DIRH_MAXPATH ) )
		return dirh_error( sf, CG_ERROR_NO_FILE_ENTRY, 0 );
	
	memset( &stat, 0, sizeof( SceIoStat ) );
	
	if( sf->curDirFullpath[strlen( sf->curDirFullpath ) - 1] != ':' ){
		/* 末尾が:じゃなければドライブではないのでGetstat実行 */
		if( ( ret = sceIoGetstat( sf->curDirFullpath, &stat ) ) < 0 )
			return dirh_error( sf, CG_ERROR_FAILED_TO_GETSTAT, ret );
		
		if( ! FIO_S_ISDIR( stat.st_mode ) )
			return dirh_error( sf, CG_ERROR_PATH_IS_NOT_DIR, 0 );
	}
	
	dirh_destroy_dirent( &(sf->entry) );
	
	return dirh_get_dirent_thread( sf->curDirFullpath, sf );
}

char* dirhRead( DirhUID uid )
{
	struct dirh_select_filename *sf = (struct dirh_select_filename *)uid;
	int dircnt = dirhGetDirEntryCount( uid );
	char *retv;
	
	if( sf->entry.pos >= dirhGetAllEntryCount( uid ) ) return NULL;
	
	if( sf->entry.pos < dircnt ){
		retv = sf->entry.dirList[sf->entry.pos];
	} else{
		retv = sf->entry.fileList[sf->entry.pos - dircnt];
	}
	
	sf->entry.pos++;
	return retv;
}

int dirhTell( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->entry.pos;
}

void dirhSeek( DirhUID uid, DirhWhence whence, int count )
{
	int allent = dirhGetAllEntryCount( uid );
	struct dirh_select_filename *sf = (struct dirh_select_filename *)uid;
	
	if( whence == DW_SEEK_SET ){
		sf->entry.pos = 0;
	} else if( whence == DW_SEEK_END ){
		sf->entry.pos = allent;
	} else if( whence == DW_SEEK_DIR ){
		sf->entry.pos = 0;
	} else if( whence == DW_SEEK_FILE ){
		sf->entry.pos = dirhGetDirEntryCount( uid );
	}
	
	sf->entry.pos += count;
	
	if( sf->entry.pos < 0 ){
		sf->entry.pos = 0;
	} else if( sf->entry.pos > allent ){
		sf->entry.pos = allent;
	}
}

char* dirhGetCwd( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->curDirFullpath;
}

char* dirhGetFileName( DirhUID uid, int idxnum )
{
	int dircnt = dirhGetDirEntryCount( uid );
	
	if( idxnum < dircnt ){
		return ((struct dirh_select_filename *)uid)->entry.dirList[idxnum];
	} else if( idxnum < dirhGetAllEntryCount( uid ) ){
		return ((struct dirh_select_filename *)uid)->entry.fileList[idxnum - dircnt];
	} else{
		return NULL;
	}
}

unsigned int dirhGetFileType( DirhUID uid, int idxnum )
{
	if( idxnum < dirhGetDirEntryCount( uid ) ){
		return FIO_SO_IFDIR;
	} else if( idxnum < dirhGetAllEntryCount( uid ) ){
		return FIO_SO_IFREG;
	} else{
		return 0;
	}
}

int dirhGetAllEntryCount( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->entry.dirCount + ((struct dirh_select_filename *)uid)->entry.fileCount;
}

int dirhGetDirEntryCount( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->entry.dirCount;
}

int dirhGetFileEntryCount( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->entry.fileCount;
}

int dirhGetLastError( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->lError;
}

int dirhGetLastSystemError( DirhUID uid )
{
	return ((struct dirh_select_filename *)uid)->sError;
}
