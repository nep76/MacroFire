/*
	padutil.c
*/

#include "padutil.h"
/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
PadutilButtons st_btndef[] = {
	{ PSP_CTRL_SELECT,           "SELECT"      },
	{ PSP_CTRL_START,            "START"       },
	{ PSP_CTRL_UP,               "UP"          },
	{ PSP_CTRL_RIGHT,            "RIGHT"       },
	{ PSP_CTRL_DOWN,             "DOWN"        },
	{ PSP_CTRL_LEFT,             "LEFT"        },
	{ PSP_CTRL_LTRIGGER,         "LTRIGGER"    },
	{ PSP_CTRL_RTRIGGER,         "RTRIGGER"    },
	{ PSP_CTRL_TRIANGLE,         "TRIANGLE"    },
	{ PSP_CTRL_CIRCLE,           "CIRCLE"      },
	{ PSP_CTRL_CROSS,            "CROSS"       },
	{ PSP_CTRL_SQUARE,           "SQUARE"      },
	{ PSP_CTRL_HOME,             "HOME"        },
	{ PSP_CTRL_HOLD,             "HOLD"        },
	{ PSP_CTRL_NOTE,             "NOTE"        },
	{ PSP_CTRL_SCREEN,           "SCREEN"      },
	{ PSP_CTRL_VOLUP,            "VOLUP"       },
	{ PSP_CTRL_VOLDOWN,          "VOLDOWN"     },
	{ PSP_CTRL_WLAN_UP,          "WLANUP"      },
	{ PSP_CTRL_REMOTE,           "REMOTE"      },
	{ PSP_CTRL_DISC,             "DISC"        },
	{ PSP_CTRL_MS,               "MS"          },
	{ PADUTIL_CTRL_ANALOG_UP,    "ANALOGUP"    },
	{ PADUTIL_CTRL_ANALOG_RIGHT, "ANALOGRIGHT" },
	{ PADUTIL_CTRL_ANALOG_DOWN,  "ANALOGDOWN"  },
	{ PADUTIL_CTRL_ANALOG_LEFT,  "ANALOGLEFT"  },
	{ 0, NULL }
};
/*=============================================*/

char *padutilGetButtonNamesByCode( PadutilButtons *availbtn, unsigned int buttons, const char *delim, unsigned int opt, char *buf, size_t len )
{
	int i;
	char *delimtoken = NULL;
	
	if( ! buf || ! len ) return NULL;
	if( ! buttons  ){
		*buf = '\0';
		return buf;
	}
	if( ! availbtn ) availbtn = st_btndef;
	
	if( delim ){
		delimtoken = memsceMalloc( strlen( delim ) + 2 + 1 );
		if( ! delimtoken ) return NULL;
		
		strcpy( delimtoken, delim );
		
		if( opt & PADUTIL_OPT_IGNORE_SP ){
			strutilRemoveChar( delimtoken, "\x20\t" );
		}
		if( opt & PADUTIL_OPT_TOKEN_SP ){
			int len = strlen( delimtoken );
			memmove( delimtoken + 1, delimtoken, len + 1 );
			delimtoken[0]       = '\x20';
			delimtoken[len + 1] = '\x20';
			delimtoken[len + 2] = '\0';
		}
	}
	
	*buf = '\0';
	
	for( i = 0; availbtn[i].button || availbtn[i].name; i++ ){
		if( buttons & availbtn[i].button ){
			if( *buf != '\0' && delimtoken ){
				strutilSafeCat( buf, delimtoken, len );
			}
			if( availbtn[i].name ) strutilSafeCat( buf, availbtn[i].name, len );
		}
	}
	
	memsceFree( delimtoken );
	
	return buf;
}

unsigned int padutilGetButtonCodeByNames( PadutilButtons *availbtn, const char *names, const char *delim, unsigned int opt )
{
	int i, nameslen = 0, delimlen = 0;
	unsigned int buttons = 0;
	char *btn_name, *next = NULL;
	
	void *comparer;
	char *namelist, *delimtoken;
	
	if( ! names || names[0] == '\0' ) return 0;
	if( ! availbtn ) availbtn = st_btndef;
	if( delim      ) delimlen = strlen( delim );
	
	nameslen = strlen( names );
	
	namelist = memsceMalloc( nameslen + delimlen + 2 );
	if( ! namelist ) return 0;
	
	delimtoken = namelist + nameslen + 1;
	
	strcpy( namelist, names );
	strcpy( delimtoken, delim );
	
	if( opt & PADUTIL_OPT_IGNORE_SP ){
		strutilRemoveChar( namelist,   "\x20\t" );
		strutilRemoveChar( delimtoken, "\x20\t" );
	}
	if( opt & PADUTIL_OPT_CASE_SENS ){
		comparer = strcmp;
	} else{
		comparer = strcasecmp;
	}
	
	btn_name = namelist;
	
	do{
		if( delimlen ){
			next = strstr( btn_name, delimtoken );
			if( next ){
				*next = '\0';
				next += delimlen;
			}
		}
		
		for( i = 0; availbtn[i].button || availbtn[i].name; i++ ){
			if( availbtn[i].name && ( ((int (*)(const char*, const char*))comparer)( btn_name, availbtn[i].name ) == 0 ) ){
				buttons |= availbtn[i].button;
				break;
			}
		}
		btn_name = next;
	} while( btn_name );
	
	memsceFree( namelist );
	
	return buttons;
}

unsigned int padutilGetAnalogDirection( int x, int y, int deadzone )
{
	unsigned int direction = 0;
	
	x -= PADUTIL_CENTER_X;
	y -= PADUTIL_CENTER_Y;
	
	if( deadzone ){
		if( PADUTIL_IN_DEADZONE( abs( x ), abs( y ), deadzone ) ) return 0;
	} else{
		if( x == 0 && y == 0 ) return 0;
	}
	
	if( abs( y ) > (int)(sin( PADUTIL_DEGREE_TO_RADIAN( PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE ) ) * abs( x )) ){
		if( y > 0 ){
			direction |= PADUTIL_CTRL_ANALOG_DOWN;
		} else if( y < 0 ){
			direction |= PADUTIL_CTRL_ANALOG_UP;
		}
	}
	
	if( abs( x ) > (int)(sin( PADUTIL_DEGREE_TO_RADIAN( PADUTIL_INVALID_RIGHT_TRIANGLE_DEGREE ) ) * abs( y )) ){
		if( x > 0 ){
			direction |= PADUTIL_CTRL_ANALOG_RIGHT;
		} else if( x < 0 ){
			direction |= PADUTIL_CTRL_ANALOG_LEFT;
		}
	}
	
	return direction;
}

void padutilSetAnalogDirection( unsigned int analog_direction, unsigned char *x, unsigned char *y )
{
	if( analog_direction & PADUTIL_CTRL_ANALOG_UP    ) *y = 0;
	if( analog_direction & PADUTIL_CTRL_ANALOG_RIGHT ) *x = 255;
	if( analog_direction & PADUTIL_CTRL_ANALOG_DOWN  ) *y = 255;
	if( analog_direction & PADUTIL_CTRL_ANALOG_LEFT  ) *x = 0;
}
