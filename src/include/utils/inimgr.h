/*
	INIマネージャ
*/

#ifndef INIMGR_H
#define INIMGR_H

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "utils/strutil.h"
#include "psp/fileh.h"
#include "cgerrs.h"

/*-----------------------------------------------
	定数
-----------------------------------------------*/
#define INIMGR_DEFAULT_SECTION_NAME "default" /* 63文字以下でなければならない */
#define INIMGR_SECTION_BUFFER  64
#define INIMGR_ENTRY_BUFFER   256

#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------
	型宣言
-----------------------------------------------*/
typedef int  IniUID;

enum inimgr_line_type {
	INIMGR_LINE_NULL = 0,
	INIMGR_LINE_SECTION,
	INIMGR_LINE_ENTRY
};

typedef enum {
	INIMGR_CB_LOAD = 0,
	INIMGR_CB_SAVE
} InimgrCallbackMode;

struct inimgr_callback {
	char section[INIMGR_SECTION_BUFFER];
	void *cb;
	SceOff offset;
	unsigned int length;
	struct inimgr_callback *next;
};

struct inimgr_entry {
	char   *key;
	char   *value;
	size_t vspace;
	struct inimgr_entry *prev, *next;
};

struct inimgr_section {
	char name[INIMGR_SECTION_BUFFER];
	struct inimgr_entry *entry;
	struct inimgr_section *prev, *next;
};

struct inimgr_params {
	struct inimgr_callback *callbacks; /* コールバック設定 */
	struct inimgr_section  *index;     /* セクションのリンクリスト */
	struct inimgr_section  *last;      /* 常に終端のセクションを指す */
};
	
typedef struct {
	IniUID uid;
	FilehUID ini;
	struct inimgr_callback *cbinfo;
} InimgrCallbackParams;

typedef void ( *InimgrCallback )( InimgrCallbackMode, InimgrCallbackParams*, char*, size_t );

/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/

/*
	INIマネージャを初期化し、INIハンドルを得る
	
	@return IniUID
		>= 0: 有効なINIハンドル
		< 0 : 失敗
*/
IniUID inimgrNew( void );

/*
	通常のINIファイル仕様に存在しない拡張機能。
	これにより、key = paramという形式以外のデータや、
	INIマネージャを通さず直接ファイルを読んでデータを読むことで省メモリにできる。
	
	このセクションを便宜上スペシャルセクションと呼ぶ。
	
	次に出現するセクションまでの全てのデータはINIマネージャの手を離れるため、
	INIファイル中に全く異なるフォーマットのデータを唐突に出現させることができてしまうことや、
	セクションごとにしか設定できないため、セクションが細かく分かれてしまうなどのデメリットもある。
	
	inimgrLoad()/inimgrSave()によるINIファイルの読み込み/保存時に、
	コールバック関数を呼び出すセクション名を指定する。
	
	inimgrSave()は、これにより設定されたセクション名の記録を開始する時に、コールバック関数を呼び出す。
	コールバック関数はinimgrMakeEntry()やinimgrWriteln()などを使って設定値を書き出す。
	
	inimgrLoad()は、これにより設定されたセクション名を探し当てると、その位置を記録する。
	その後、一度INIファイルを全て読み出した後、コールバック関数を呼び出す。
	従って、コールバック関数内から、INIファイル中の他の設定値を呼び出すことは可能。
	ただし、他のスペシャルセクションは発見した順に処理されるため、読めるかどうかはファイルの内容による。
	コールバック関数はinimgrReadln()やinimgrParseEntry()を使って必要な情報を抽出する。
	
	inimgrSave()から呼び出されたコールバック関数では、inimgrSeek*()を使ってはいけない。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		コールバック関数と関連付けるセクション名。
	
	@param InimgrLoadCallback cb
		コールバック関数。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int inimgrSetCallback( IniUID uid, const char *section, InimgrCallback cb );

/*
	INIファイルを読み込み、設定内容をメモリに取り込む。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *inipath
		読み込むINIファイルのパス。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int  inimgrLoad( IniUID uid, const char *inipath );

/*
	メモリ上のINI情報をファイルへ書き出す。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *inipath
		書き込み先のINIファイルのパス。
		すでに存在するファイルであれば上書きし、存在しなければ作成する。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int  inimgrSave( IniUID uid, const char *inipath );

/*
	INIハンドルとそれに対応するINI情報を破棄する。
	対象のINIハンドルは無効になるため、使ってはいけない。
	
	@param IniUID uid
		INIハンドル。
*/
void inimgrDestroy( IniUID uid );

/*
	現在のINI情報に新しいセクションを追加/削除する。
	セクション名は大文字と小文字を区別しない。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		セクション名。
	
	@return int
		0  : 成功
		< 0: 失敗
*/
int inimgrAddSection( IniUID uid, const char *section );
void inimgrDeleteSection( IniUID uid, const char *section );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を取得する。
	
	セクション、あるいはキーが存在しない場合は偽が返り、値のセットは行わない。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param const long *var
		検索対象の値をセットする変数のポインタ。
	
	@return bool
		検索対象が見つかった場合に真、そうでなければ偽。
*/	
bool inimgrGetInt( IniUID uid, const char *section, const char *key, long *var );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を文字列として指定されたバッファにコピーする。
	
	セクション、あるいはキーが存在しない場合は-1が返り、値のセットは行わない。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param char *buf
		検索対象の文字列をコピーするバッファ。
	
	@param size_t bufsize
		上記のバッファのサイズ。
	
	@return unsigned int
		バッファにコピーされたバイト数。
*/
int inimgrGetString( IniUID uid, const char *section, const char *key, char *buf, size_t bufsize );

/*
	INI情報から指定されたセクション内にある指定されたキーを検索し、
	それに関連づけられている値を文字列として扱い、
	ONと等しければtrueを、OFFと等しければfalseを返す。
	
	ON/OFFは大文字と小文字を区別しない。
	
	セクション、あるいはキーが存在しない、または値がON/OFFのいずれでもない場合は偽が返り、値のセットは行わない。
	
	@param IniUID uid
		INIハンドル。
	
	@param const char *section
		検索対象のセクション名。
	
	@param const char *key
		検索対象のキー名。
	
	@param const bool *var
		検索対象の値をセットする変数のポインタ。
	
	@return bool
		検索対象が見つかった場合に真、そうでなければ偽。
*/	
bool inimgrGetBool( IniUID uid, const char *section, const char *key, bool *var );

/*
	inimgrSetInt
	inimgrSetString
	inimgrSetBool
	
	セクションに対してキーを設定する。
	すでに存在する場合はキーの値を更新する。
	
	キー名は大文字と小文字を区別しない。
*/
int inimgrSetInt( IniUID uid, const char *section, const char *key, const long num );
int inimgrSetString( IniUID uid, const char *section, const char *key, const char *str );
int inimgrSetBool( IniUID uid, const char *section, const char *key, const bool on );

int inimgrCbReadln( InimgrCallbackParams *params, char *buf, size_t buflen );
int inimgrCbSeekSet( InimgrCallbackParams *params, SceOff offset );
int inimgrCbSeekCur( InimgrCallbackParams *params, SceOff offset );
int inimgrCbSeekEnd( InimgrCallbackParams *params, SceOff offset );
int inimgrCbTell( InimgrCallbackParams *params );
int inimgrCbWriteln( InimgrCallbackParams *params, char *buf, size_t buflen );
bool inimgrParseEntry( char *entry, char **key, char **value );
bool inimgrMakeEntry( char *entry, char *key, char *value );



#ifdef __cplusplus
}
#endif

#endif
