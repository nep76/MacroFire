/*
	confmgr.c
*/

#include "confmgr.h"

int confmgrStore( ConfmgrHandler *handlers, int count, const char *fullpath )
{
	FilehParams params;
	FilehUID fuid;
	char pathbuf[strlen( fullpath ) + 1];
	char line[CONFMGR_LINE_BUFFER], *buf;
	int i, linesize, bufsize, store_cnt = 0;
	
	/*
		プラグイン起動時にヒープを使うと妙なことになる場合が多いので、
		スタックを利用するためにfilehOpen2を使う。
	*/
	params.path = pathbuf;
	fuid = filehOpen2( &params, fullpath, PSP_O_RDWR | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	
	if( ! fuid ){
		return -1;
	} else if( filehGetLastError( fuid ) < 0 ){
		unsigned int ferror = filehGetLastError( fuid );
		filehDestroy( fuid );
		
		return ferror;
	}
	
	for( i = 0; i < count; i++ ){
		snprintf( line, sizeof( line ), "%s = ", handlers[i].label );
		
		linesize = strlen( line );
		buf      = line + linesize;
		bufsize  = sizeof( line ) - linesize;
		
		if( handlers[i].type == CH_LONGINT ){
			snprintf( buf, bufsize, "%ld", *((long *)handlers[i].mparam) );
		} else if( handlers[i].type == CH_STRING ){
			strutilSafeCopy( buf, (char *)(handlers[i].mparam), bufsize );
		} else if( handlers[i].type == CH_CALLBACK ){
			((ConfmgrStoreCallback)(handlers[i].handlerStore))( buf, bufsize, handlers[i].mparam, handlers[i].sparam );
		} else{
			continue;
		}
		
		filehWrite( fuid, line, strlen( line ) );
		filehWrite( fuid, "\n", 1 );
		store_cnt++;
	}
	
	filehClose2( fuid );
	
	return store_cnt;
}

int confmgrLoad( ConfmgrHandler *handlers, int count, const char *fullpath )
{
	FilehParams params;
	FilehUID fuid;
	char pathbuf[strlen( fullpath ) + 1];
	char line[CONFMGR_LINE_BUFFER], *name, *value;
	char *tmp_boundary1, *tmp_boundary2;
	int i, load_cnt = 0;
	
	/*
		プラグイン起動時にヒープを使うと妙なことになる場合が多いので、
		スタックを利用するためにfilehOpen2を使う。
	*/
	params.path = pathbuf;
	fuid = filehOpen2( &params, fullpath, PSP_O_RDONLY, 0777 );
	
	if( ! fuid ){
		return -1;
	} else{
		unsigned int ferror = filehGetLastError( fuid );
		filehDestroy( fuid );
		if( ferror == FILEH_ERROR_GETSTAT_FAILED ){
			return 0;
		} else if( ferror < 0 ){
			return ferror;
		}
	}
	
	while( filehReadln( fuid, line, sizeof( line ) ) ){
		if( line[0] == '#' || line[0] == '\0' ) continue;
		
		tmp_boundary1 = strchr( line, '=' );
		if( ! tmp_boundary1 ) continue;
		
		*tmp_boundary1 = '\0';
		name           = line;
		value          = ++tmp_boundary1;
		
		/* nameの空白文字を全て排除 */
		strutilRemoveChar( name, "\x20\t" );
		strutilToUpper( name );
		
		if( name[0] == '\0' ) continue;
		
		/* valueの先頭と末尾の空白を飛ばす */
		tmp_boundary1 = strutilCounterPbrk( value, "\x20\t" );
		tmp_boundary2 = strutilCounterReversePbrk( value, "\x20\t" );
		
		if( tmp_boundary1 == tmp_boundary2 ){
			tmp_boundary2 = NULL;
		}
		
		if( tmp_boundary1 ){
			value = tmp_boundary1;
		}
		if( tmp_boundary2 ){
			tmp_boundary2++;
			if( tmp_boundary2 != '\0' ){
				*tmp_boundary2 = '\0';
			}
		}
		
		for( i = 0; i < count; i++ ){
			if( strcmp( name, handlers[i].label ) == 0 ){
				if( handlers[i].type == CH_LONGINT ){
					*((long *)handlers[i].mparam) = strtol( value, NULL, 10 );
				} else if( handlers[i].type == CH_STRING ){
					strutilSafeCopy( (char *)(handlers[i].mparam), value, *((size_t *)handlers[i].sparam) );
				} else if( handlers[i].type == CH_CALLBACK ){
					((ConfmgrLoadCallback)(handlers[i].handlerLoad))( name, value, handlers[i].mparam, handlers[i].sparam );
				} else{
					continue;
				}
				load_cnt++;
				break;
			}
		}
	}
	
	filehClose2( fuid );
	
	return load_cnt;
}
