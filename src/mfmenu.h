/*=========================================================

	mfmenu.h

	MacroFire メニュー

=========================================================*/
#ifndef MFMENU_H
#define MFMENU_H

#define MFEXCLUDE_MENU
#include "macrofire.h"
#undef MFEXCLUDE_MENU

/*=========================================================
	マクロ
=========================================================*/
#define MF_MENU_INFOTEXT_LENGTH 256

#define mfMenuInitTable( menu )      mfMenuInitTables( menu, 1 )
#define mfMenuDrawTable( menu, opt ) mfMenuDrawTables( menu, 1, opt )

/*=========================================================
	型宣言
=========================================================*/
typedef enum {
	MF_MENU_NO_OPTIONS   = 0,
	MF_MENU_DISPLAY_ONLY = 0x00000001
} MfMenuDrawOptions;

typedef enum {
	MF_MENU_INFOTEXT_NO_OPTIONS       = 0,
	MF_MENU_INFOTEXT_SET_LOWER_LINE   = 0x00000001,
	MF_MENU_INFOTEXT_SET_MIDDLE_LINE  = 0x00000002,
	MF_MENU_INFOTEXT_ERROR            = 0x00000004,
	MF_MENU_INFOTEXT_COMMON_CTRL      = 0x00000010,
	MF_MENU_INFOTEXT_MULTICOLUMN_CTRL = 0x00000020,
	MF_MENU_INFOTEXT_MOVABLEPAGE_CTRL = 0x00000040,
} MfMenuInfoTextOptions;

typedef enum {
	MF_MENU_SCREEN_UPDATE       = 0x00000001,
	MF_MENU_FORCED_QUIT         = 0x00000002,
} MfMenuFlags;

/*=========================================================
	プロトタイプ
=========================================================*/
bool mfMenuInit( void );
void mfMenuDestroy( void );
void mfMenuMain( SceCtrlData *pad, MfHprmKey *hk );
void mfMenuIniLoad( IniUID ini, char *buf, size_t len );
void mfMenuIniSave( IniUID ini, char *buf, size_t len );
void mfMenuWait( unsigned int microsec );
void mfMenuProc( MfMenuProc proc );
bool mfMenuSetExtra( MfMenuProc extra );
void mfMenuExitExtra( void );
void mfMenuEnableQuickQuit( void );
void mfMenuDisableQuickQuit( void );
void mfMenuEnable( unsigned int flags );
void mfMenuDisable( unsigned int flags );
unsigned int mfMenuGetFlags( void );
char *mfMenuButtonsSymbolList( PadutilButtons buttons, char *str, size_t len );
bool mfMenuDrawDialog( MfDialogType dialog, bool update );
void mfMenuInitTables( MfMenuTable menu[], unsigned short menucnt );
bool mfMenuDrawTables( MfMenuTable menu[], unsigned short menucnt, unsigned int opt );
MfMenuTable *mfMenuCreateTables( unsigned short tables, ... );
void mfMenuDestroyTables( MfMenuTable *table );
void mfMenuSetTableLabel( MfMenuTable *menu, unsigned short tableid, char *label );
void mfMenuSetTablePosition( MfMenuTable *menu, unsigned short tableid, int x, int y );
void mfMenuSetColumnWidth( MfMenuTable *menu, unsigned short tableid, unsigned short col, unsigned int width );
void mfMenuSetTableEntry( MfMenuTable *menu, unsigned short tableid, unsigned short row, unsigned short col, char *label, MfControl ctrl, void *var, void *arg );
void mfMenuActiveTableEntry( MfMenuTable *menu, unsigned short tableid, unsigned short row, unsigned short col );
void mfMenuInactiveTableEntry( MfMenuTable *menu, unsigned short tableid, unsigned short row, unsigned short col );
void mfMenuSetInfoText( unsigned int options, char *format, ... );
unsigned int mfMenuAcceptButton( void );
unsigned int mfMenuCancelButton( void );
char *mfMenuAcceptSymbol( void );
char *mfMenuCancelSymbol( void );
inline SceCtrlData *mfMenuGetCurrentPadData( void );
inline unsigned int mfMenuGetCurrentButtons( void );
inline unsigned char mfMenuGetCurrentAnalogStickX( void );
inline unsigned char mfMenuGetCurrentAnalogStickY( void );
inline u32 mfMenuGetCurrentHprmKey( void );
inline void mfMenuResetKeyRepeat( void );
unsigned int mfMenuScroll( int selected, unsigned int viewlines, unsigned int maxlines );

#define mfMenuQuit()            mfMenuEnable( MF_MENU_FORCED_QUIT )
#define mfMenuUpdate()          mfMenuEnable( MF_MENU_SCREEN_UPDATE )
#define mfMenuIsEnabled( f, t ) ( ( ( ( f ) & ( t ) ) == ( t ) ? true : false ) )

#endif
