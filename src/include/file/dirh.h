/*=========================================================

	dirh.h
	
	ディレクトリハンドラ。

=========================================================*/
#ifndef DIRH_H
#define DIRH_H

#include <pspkernel.h>
#include <stdio.h>

/*=========================================================
	マクロ
=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=========================================================
	型宣言
=========================================================*/
typedef intptr_t DirhUID;

typedef enum {
	DIRH_O_DOPEN_WITH_THREAD = 0x00000001,
	DIRH_O_ALLOC_HIGH        = 0x00000002
} DirhOptions;

typedef enum {
	DIRH_NULL = 0,
	DIRH_FILE = FIO_SO_IFREG,
	DIRH_DIR  = FIO_SO_IFDIR
} DirhFileType;

typedef enum {
	DIRH_SEEK_SET = SEEK_SET,
	DIRH_SEEK_CUR = SEEK_CUR,
	DIRH_SEEK_END = SEEK_END
} DirhWhence;

typedef struct {
	char *name;
	DirhFileType type;
} DirhFileInfo;

/*=========================================================
	関数
=========================================================*/
DirhUID dirhNew( size_t pathmax, unsigned int options );
int dirhChdir( DirhUID uid, const char *dirpath, unsigned int timeout );
void dirhDestroy( DirhUID uid );
DirhFileInfo *dirhRead( DirhUID uid );
int dirhTell( DirhUID uid );
char *dirhGetCwd( DirhUID uid );
void dirhSeek( DirhUID uid, DirhWhence whence, int offset );
void dirhSort( DirhUID uid, int ( *compare )( const void*, const void* ) );
DirhFileInfo *dirhGetFileInfo( DirhUID uid, int num );

#ifdef __cplusplus
}
#endif

#endif
