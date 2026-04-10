/*=========================================================

	macromgr.c

	マクロマネージャ

=========================================================*/
#include "macromgr.h"

/*=========================================================
	ローカルマクロ
=========================================================*/
#define MACROMGR_HEAPBLOCK_SIZE 1024

/*=========================================================
	ローカル型宣言
=========================================================*/
struct macromgr_params {
	MacromgrCommand *macro;
	DmemUID dmem;
};

/*=========================================================
	ローカル関数
=========================================================*/
static MacromgrCommand *macromgr_command_new( struct macromgr_params *params );
static int macromgr_ini_parser( struct macromgr_params *params, InimgrCallbackParams *cbp, char *buf, size_t buflen, MacromgrCommand *macro );

/*=========================================================
	関数
=========================================================*/
MacromgrUID macromgrNew( void )
{
	DmemUID uid;
	struct macromgr_params *params;
	
	uid = dmemNew( 0 );
	if( ! uid ) return 0;
	
	params = (struct macromgr_params *)dmemAlloc( uid, sizeof( struct macromgr_params ) );
	if( ! params ) return 0;
	
	params->macro = NULL;
	params->dmem  = uid;
	
	return (MacromgrUID)params;
}

void macromgrDestroy( MacromgrUID uid )
{
	macromgrClear( uid );
	
	dmemDestroy( ((struct macromgr_params*)uid)->dmem );
}

MacromgrCommand *macromgrSeek( MacromgrUID uid, unsigned int offset, MacromgrWhence whence, MacromgrCommand *cur )
{
	struct macromgr_params *params = (struct macromgr_params *)uid;
	MacromgrCommand *cmd = NULL;
	
	switch( whence ){
		case MACROMGR_SEEK_SET:
			cmd = params->macro;
			while( offset-- ) cmd = cmd->next;
			break;
		case MACROMGR_SEEK_END:
			cmd = params->macro;
			while( cmd->next ) cmd = cmd->next;
			while( offset-- ) cmd = cmd->prev;
			break;
		case MACROMGR_SEEK_CUR:
			cmd = cur;
			while( offset--) cmd = cmd->next;
			break;
	}
	
	return cmd;
}

inline MacromgrCommand *macromgrNext( MacromgrCommand *macro )
{
	return macro->next;
}

inline MacromgrCommand *macromgrPrev( MacromgrCommand *macro )
{
	return macro->prev;
}

MacromgrCommand *macromgrCreateRoot( MacromgrUID uid )
{
	struct macromgr_params *params = (struct macromgr_params *)uid;
	
	params->macro = macromgr_command_new( params );
	if( ! params->macro ) return NULL;
	
	return params->macro;
}

MacromgrCommand *macromgrInsertBefore( MacromgrUID uid, MacromgrCommand *macro )
{
	struct macromgr_params *params = (struct macromgr_params *)uid;
	MacromgrCommand *newcmd;
	
	newcmd = macromgr_command_new( params );
	if( ! newcmd ) return NULL;
	
	/* 再リンク */
	if( ! macro->prev ) params->macro = newcmd;
	
	newcmd->next = macro;
	newcmd->prev = macro->prev;
	if( macro->prev ) macro->prev->next = newcmd;
	macro->prev = newcmd;
	
	return newcmd;
}

MacromgrCommand *macromgrInsertAfter( MacromgrUID uid, MacromgrCommand *macro )
{
	struct macromgr_params *params = (struct macromgr_params *)uid;
	MacromgrCommand *newcmd;
	
	newcmd = macromgr_command_new( params );
	if( ! newcmd ) return NULL;
	
	/* 再リンク */
	newcmd->next = macro->next;
	newcmd->prev = macro;
	if( macro->next ) macro->next->prev = newcmd;
	macro->next = newcmd;
	
	return newcmd;
}

bool macromgrRemove( MacromgrUID uid, MacromgrCommand *macro )
{
	struct macromgr_params *params = (struct macromgr_params *)uid;
	
	/* 前も後ろも無ければひとりぼっちなので何もしない */
	if( ! macro->prev && ! macro->next ) return true;
	
	/* マクロの再リンク */
	if( macro->next ) macro->next->prev = macro->prev;
	if( macro->prev ){
		macro->prev->next = macro->next;
	} else{
		/* 削除対象マクロの前がなければ先頭なので、先頭を再リンク */
		params->macro = macro->next;
	}
	
	dmemFree( params->dmem, macro );
	return true;
}

void macromgrClear( MacromgrUID uid )
{
	struct macromgr_params *params = (struct macromgr_params *)uid;
	
	if( params->macro ){
		MacromgrCommand *prev = params->macro;
		MacromgrCommand *cur  = prev->next;
		
		while( cur ){
			dmemFree( params->dmem, prev );
			prev = cur;
			cur  = cur->next;
		}
		
		dmemFree( params->dmem, prev );
	}
	params->macro = NULL;
}

void macromgrSetCommand( MacromgrCommand *macro, MacromgrAction action, MacromgrData data, MacromgrData sub )
{
	macro->action = action;
	macro->data   = data;
	macro->sub    = sub;
}

int macromgrLoader( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg )
{
	struct macromgr_params *params = *((struct macromgr_params **)arg);
	
	macromgrCreateRoot( (MacromgrUID)params );
	
	return macromgr_ini_parser( params, cbp, buf, buflen, params->macro );
}

int macromgrAppendLoader( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg )
{
	struct macromgr_params *params = *((struct macromgr_params **)arg);
	MacromgrCommand *lastmacro;
	int ret;
	
	if( ! params->macro ) macromgrCreateRoot( (MacromgrUID)params );
	
	lastmacro = macromgrSeek( (MacromgrUID)params, 0, MACROMGR_SEEK_END, NULL );
	
	lastmacro->next = macromgr_command_new( params );
	if( lastmacro->next ){
		lastmacro->next->prev = lastmacro;
		lastmacro             = lastmacro->next;
		ret = macromgr_ini_parser( params, cbp, buf, buflen, lastmacro );
	} else{
		ret = CG_ERROR_NOT_ENOUGH_MEMORY;
	}
	
	return ret;
}

int macromgrSaver( InimgrCallbackMode mode, InimgrCallbackParams *cbp, char *buf, size_t buflen, void *arg )
{
	struct macromgr_params *params = *((struct macromgr_params **)arg);
	char buttons[128];
	unsigned int len = 0;
	MacromgrCommand *cmd;
	
	if( ! mfConvertButtonReady() ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	for( cmd = params->macro; cmd; cmd = cmd->next ){
		switch( cmd->action ){
			case MACROMGR_DELAY:
				len = snprintf( buf, buflen, "%s = %llu", MACROMGR_INI_KEY_DELAY, (MacromgrData)cmd->data );
				break;
			case MACROMGR_BUTTONS_PRESS:
				mfConvertButtonC2N( cmd->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%s = %s", MACROMGR_INI_KEY_BUTTONS_PRESS, buttons );
				break;
			case MACROMGR_BUTTONS_RELEASE:
				mfConvertButtonC2N( cmd->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%s = %s", MACROMGR_INI_KEY_BUTTONS_RELEASE, buttons );
				break;
			case MACROMGR_BUTTONS_CHANGE:
				mfConvertButtonC2N( cmd->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%s = %s", MACROMGR_INI_KEY_BUTTONS_CHANGE, buttons );
				break;
			case MACROMGR_ANALOG_MOVE:
				len = snprintf( buf, buflen, "%s = %d,%d", MACROMGR_INI_KEY_ANALOG_MOVE, MACROMGR_GET_ANALOG_X( cmd->data ), MACROMGR_GET_ANALOG_Y( cmd->data ) );
				break;
			case MACROMGR_RAPIDFIRE_START:
				mfConvertButtonC2N( cmd->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%s = %d,%d,%s", MACROMGR_INI_KEY_RAPIDFIRE_START, MACROMGR_GET_RAPIDPDELAY( cmd->sub ), MACROMGR_GET_RAPIDRDELAY( cmd->sub ), buttons );
				break;
			case MACROMGR_RAPIDFIRE_STOP:
				mfConvertButtonC2N( cmd->data, buttons, sizeof( buttons ) );
				len = snprintf( buf, buflen, "%s = %s", MACROMGR_INI_KEY_RAPIDFIRE_STOP, buttons );
				break;
			default:
				continue;
		}
		inimgrCbWriteln( cbp, buf, len );
	}
	
	mfConvertButtonFinish();
	
	return 0;
}

bool macromgrGetCommand( MacromgrCommand *macro, MacromgrAction *action, MacromgrData *data, MacromgrData *sub )
{
	if( ! macro ) return false;
	
	*action = macro->action;
	*data   = macro->data;
	if( sub  ) *sub = macro->sub;
	
	return true;
}

static MacromgrCommand *macromgr_command_new( struct macromgr_params *params )
{
	MacromgrCommand *newdata;
	
	newdata = (MacromgrCommand *)dmemAlloc( params->dmem, sizeof( MacromgrCommand ) );
	newdata->action = MACROMGR_DELAY;
	newdata->data   = 0;
	newdata->sub    = 0;
	newdata->next   = NULL;
	newdata->prev   = NULL;
	
	return newdata;
}

static int macromgr_ini_parser( struct macromgr_params *params, InimgrCallbackParams *cbp, char *buf, size_t buflen, MacromgrCommand *macro )
{
	char *key, *value;
	
	if( ! mfConvertButtonReady() ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	while( inimgrCbReadln( cbp, buf, buflen ) ){
		strutilRemoveChar( buf, "\x20\t" );
		
		if( ! inimgrParseEntry( buf, &key, &value ) ) continue;
		
		if( strcasecmp( key, MACROMGR_INI_KEY_DELAY ) == 0 ){
			macro->action = MACROMGR_DELAY;
			macro->data   = strtoul( value, NULL, 10 );
			macro->sub    = 0;
		} else if( strcasecmp( key, MACROMGR_INI_KEY_BUTTONS_PRESS ) == 0 ){
			macro->action = MACROMGR_BUTTONS_PRESS;
			macro->data   = mfConvertButtonN2C( value );
			macro->sub    = 0;
		} else if( strcasecmp( key, MACROMGR_INI_KEY_BUTTONS_RELEASE ) == 0 ){
			macro->action = MACROMGR_BUTTONS_RELEASE;
			macro->data   = mfConvertButtonN2C( value );
			macro->sub    = 0;
		} else if( strcasecmp( key, MACROMGR_INI_KEY_BUTTONS_CHANGE ) == 0 ){
			macro->action = MACROMGR_BUTTONS_CHANGE;
			macro->data   = mfConvertButtonN2C( value );
			macro->sub    = 0;
		} else if( strcasecmp( key, MACROMGR_INI_KEY_ANALOG_MOVE ) == 0 ){
			unsigned char x, y;
			char *token, *saveptr = NULL;
			token = strtok_r( value, ",", &saveptr );
			x = token ? strtoul( token, NULL, 10 ) : PADUTIL_CENTER_X;
			token = strtok_r( NULL,  ",", &saveptr );
			y = token ? strtoul( token, NULL, 10 ) : PADUTIL_CENTER_Y;
			macro->action = MACROMGR_ANALOG_MOVE;
			macro->data   = MACROMGR_SET_ANALOG_XY( x, y );
			macro->sub    = 0;
		} else if( strcasecmp( key, MACROMGR_INI_KEY_RAPIDFIRE_START ) == 0 ){
			unsigned int buttons;
			unsigned short pd, rd;
			char *token, *saveptr = NULL;
			token = strtok_r( value, ",", &saveptr );
			pd = token ? strtoul( token, NULL, 10 ) : MF_RAPIDFIRE_DEFAULT_PRESS_DELAY;
			token = strtok_r( value, ",", &saveptr );
			rd = token ? strtoul( token, NULL, 10 ) : MF_RAPIDFIRE_DEFAULT_RELEASE_DELAY;
			token = strtok_r( value, ",", &saveptr );
			buttons = mfConvertButtonN2C( token );
			macro->action = MACROMGR_RAPIDFIRE_START;
			macro->data   = buttons;
			macro->sub    = 0;
		} else if( strcasecmp( key, MACROMGR_INI_KEY_RAPIDFIRE_STOP ) == 0 ){
			macro->action = MACROMGR_RAPIDFIRE_STOP;
			macro->data   = mfConvertButtonN2C( value );
			macro->sub    = 0;
		}
		
		macro->next = macromgr_command_new( params );
		if( ! macro->next ) return CG_ERROR_NOT_ENOUGH_MEMORY;
		
		macro->next->prev = macro;
		macro = macro->next;
	}
	
	mfConvertButtonFinish();
	
	return 0;
}
