/*
	Directory Handler
*/

#ifndef DIRH_H
#define DIRH_H

#include <pspkernel.h>
#include <pspsdk.h>
#include "psp/memsce.h"
#include "psp/pathexpand.h"
#include "psp/blit.h"
#include "cgerrs.h"

#define DIRH_MAXPATH 256

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int DirhUID;

struct dirh_dirents {
	int pos;
	char **dirList;
	int dirCount;
	char **fileList;
	int fileCount;
};

typedef enum {
	DIRH_OPT_ADD_DIR_SLASH = 0x00000001,
} DirhOptions;

struct dirh_select_filename {
	char curDirFullpath[DIRH_MAXPATH];
	DirhOptions options;
	struct dirh_dirents entry;
	int lError;
	int sError;
};

typedef enum {
	DFT_ERR,
	DFT_REG,
	DFT_DIR
} DirhFileType;

typedef enum {
	DW_SEEK_SET,
	DW_SEEK_CUR,
	DW_SEEK_END,
	DW_SEEK_DIR,
	DW_SEEK_FILE,
} DirhWhence;

/* 相対パス、あるいは絶対パスで指定されたディレクトリのリストを取得 */
DirhUID dirhNew( const char *dirpath, unsigned int options );

/* オプションをそのままに別にディレクトリを相対パス、あるいは絶対パス再取得 */
int dirhChdir( DirhUID uid, const char *dirpath );

/* 取得したデータを全て破棄 */
void dirhDestroy( DirhUID uid );

/* ディレクトリエントリを読み出す */
char* dirhRead( DirhUID uid );

/* 現在のディレクトリエントリ位置を先頭からのインデックス番号で返す */
int dirhTell( DirhUID uid );

/* ディレクトリエントリ位置を移動する */
void dirhSeek( DirhUID uid, DirhWhence whence, int count );

/* 現在開いているディレクトリのフルパスを返す */
char* dirhGetCwd( DirhUID uid );

/* ディレクトリエントリ内のインデックス番号にあるファイルの名前を返す */
char* dirhGetFileName( DirhUID uid, int idxnum );

/* ディレクトリエントリ内のインデックス番号にあるファイルの属性を返す */
unsigned int dirhGetFileType( DirhUID uid, int idxnum );

int dirhGetAllEntryCount( DirhUID uid );
int dirhGetDirEntryCount( DirhUID uid );
int dirhGetFileEntryCount( DirhUID uid );

int dirhGetLastError( DirhUID uid );
int dirhGetLastSystemError( DirhUID uid );

#ifdef __cplusplus
}
#endif

#endif
