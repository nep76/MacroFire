/*
	dirh.c
*/

#include "dirh.h"

static int dirh_error( struct dirh_select_filename *sf, int lerror, int serror )
{
	sf->lError = lerror;
	sf->sError = serror;
	return sf->lError;
}

static int dirh_get_dirent( const char *dirpath, struct dirh_select_filename *sf )
{
	SceUID dfd;
	SceIoDirent dir;
	int ret, cnt_file = 0, cnt_dir = 0;
	int fi = 0, di = 0;
	unsigned int k1 = pspSdkSetK1( 0 );
	
	/* エントリ数を数える */
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
	
	/* sceIoDread()がエラーを返していればfalse */
	if( ret < 0 ) return dirh_error( sf, CG_ERROR_FAILED_TO_DREAD, ret );
	
	/* 読み込む準備 */
	sf->entry.dirList = (char **)memsceMalloc( sizeof( char* ) * cnt_dir );
	if( ! sf->entry.dirList ) goto DESTROY;
	sf->entry.fileList = (char **)memsceMalloc( sizeof( char* ) * cnt_file );
	if( ! sf->entry.fileList ) goto DESTROY;
	sf->entry.dirCount  = cnt_dir;
	sf->entry.fileCount = cnt_file;
	
	/* 読み込む */
	dfd = sceIoDopen( dirpath );
	if( dfd < 0 ){
		dirh_error( sf, CG_ERROR_FAILED_TO_DOPEN, dfd );
		goto DESTROY;
	}
	
	memset( &dir, 0, sizeof( SceIoDirent ) );
	
	for( fi = 0, di = 0; sceIoDread( dfd, &dir ) > 0;  ){
		if( FIO_S_ISDIR( dir.d_stat.st_mode ) ){
			if( strcmp( dir.d_name, "." ) == 0 ) continue;
			
			/*
				NULLとディレクトリを表す"/"のために2バイト多く確保。
				DIRH_OPT_ADD_DIR_SLASHで"/"を付加する設定になっていなくても関係なしに2バイト多く確保。
			*/
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
	pspSdkSetK1( k1 );
	return 0;
	
	DESTROY:
		memsceFree( sf->entry.fileList );
		memsceFree( sf->entry.dirList );
		if( dfd >= 0 ) sceIoDclose( dfd );
		pspSdkSetK1( k1 );
		return sf->lError;
}

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
	if( ! sf ) return 0;
	
	sf->curDirFullpath[0] = '\0';
	sf->options           = options;
	
	sf->entry.pos       = 0;
	sf->entry.dirList   = NULL;
	sf->entry.dirCount  = 0;
	sf->entry.fileList  = NULL;
	sf->entry.fileCount = 0;
	
	dirhChdir( (DirhUID)sf, dirpath );
	
	return (DirhUID)sf;
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
	return dirh_get_dirent( sf->curDirFullpath, sf );
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
