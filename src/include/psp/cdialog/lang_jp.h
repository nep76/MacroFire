/*=========================================================
	
	lang_jp.h
	
	MacroFire内で使用する文字列定数。
	
=========================================================*/
#ifndef CDIALOG_STRINGS
#define CDIALOG_STRINGS

#define CDIALOG_STR_HELP_LABEL "ヘルプ"

#define CDIALOG_STR_MESSAGE_YES "はい"
#define CDIALOG_STR_MESSAGE_NO  "いいえ"
#define CDIALOG_STR_MESSAGE_OK  "OK"

#define CDIALOG_STR_DETECTBUTTONS_DESC            "1つ選んでください"
#define CDIALOG_STR_DETECTBUTTONS_START           "  " PB_SYM_PSP_CIRCLE "  : ボタン検出開始"
#define CDIALOG_STR_DETECTBUTTONS_CLEAR           "  " PB_SYM_PSP_SQUARE "  : 現在のボタンをクリア"
#define CDIALOG_STR_DETECTBUTTONS_CANCEL          "  " PB_SYM_PSP_CROSS  "  : キャンセル"
#define CDIALOG_STR_DETECTBUTTONS_ACCEPT          "START: 決定"
#define CDIALOG_STR_DETECTBUTTONS_CURRENT_BUTTONS "現在のボタン:"
#define CDIALOG_STR_DETECTBUTTONS_DETECT_DESC     "組み合わせたいボタンを押してください。\n\n既に検出されているボタンをもう一度押すと終了します。"
#define CDIALOG_STR_DETECTBUTTONS_NOW_DETECTING   "検出中..."

#define CDIALOG_STR_GETFILENAME_SAVE_IN                 "セーブ"
#define CDIALOG_STR_GETFILENAME_LOOK_IN                 "ロード"
#define CDIALOG_STR_GETFILENAME_FILENAME                "ファイル名:"
#define CDIALOG_STR_GETFILENAME_CONFIRM_CREATE_LABEL    "作成確認"
#define CDIALOG_STR_GETFILENAME_CONFIRM_CREATE          "指定されたファイルが見つかりません。\n作成しますか?"
#define CDIALOG_STR_GETFILENAME_CONFIRM_OVERWRITE_LABEL "上書き確認"
#define CDIALOG_STR_GETFILENAME_CONFIRM_OVERWRITE       "指定されたファイル名は既に存在します。\n上書きしますか?"
#define CDIALOG_STR_GETFILENAME_ERROR_INVALID_FILENAME  "不正なファイル名"
#define CDIALOG_STR_GETFILENAME_ERROR_ILLEGAL_CHAR      "使用できない文字が含まれています。\n\n使用できない文字: \\/:?\"<>|"
#define CDIALOG_STR_GETFILENAME_ERROR_EMPTY             "ファイル名を入力してください。"
#define CDIALOG_STR_GETFILENAME_ERROR_SAME_AS_DIR_NAME  "指定されたファイル名は、既にディレクトリ名として使用されています。"
#define CDIALOG_STR_GETFILENAME_ERROR_NOT_FOUND         "ファイルが見つかりません。"
#define CDIALOG_STR_GETFILENAME_INPUT                   "ファイル名"
#define CDIALOG_STR_GETFILENAME_EXTRA_MENU_LABEL        "エクストラメニュー"
#define CDIALOG_STR_GETFILENAME_EXTRA_MENU              "未実装です。"
#define CDIALOG_STR_GETFILENAME_HELP \
	"L/R   = フォーカスの移動\n" \
	"START = 決定\n" \
	PB_SYM_PSP_CROSS "     = キャンセル\n\n" \
	"*ファイルリスト\n\n" \
	"  " PB_SYM_PSP_UP   PB_SYM_PSP_DOWN  "     = 移動\n" \
	"  " PB_SYM_PSP_LEFT PB_SYM_PSP_RIGHT "     = ページ移動\n" \
	"  " PB_SYM_PSP_CIRCLE               "      = 開く\n" \
	"  " PB_SYM_PSP_TRIANGLE             "      = 1つ上のディレクトリ\n" \
	"  SELECT = " CDIALOG_STR_GETFILENAME_EXTRA_MENU_LABEL " (未実装)\n\n" \
	"*ファイル名\n\n" \
	"  " PB_SYM_PSP_CIRCLE " = 入力"

#define CDIALOG_STR_NUMEDIT_HELP \
	PB_SYM_PSP_LEFT PB_SYM_PSP_RIGHT " = 移動\n" \
	PB_SYM_PSP_UP   PB_SYM_PSP_DOWN  " = 値の変更\n\n" \
	PB_SYM_PSP_CIRCLE " = 決定\n" \
	PB_SYM_PSP_CROSS  " = キャンセル"

#define CDIALOG_STR_SOSK_CANCEL_LABEL "キャンセル"
#define CDIALOG_STR_SOSK_CANCEL       "入力を中断しますか?"
#define CDIALOG_STR_SOSK_HELP \
	PB_SYM_PSP_UP PB_SYM_PSP_RIGHT PB_SYM_PSP_DOWN PB_SYM_PSP_LEFT " = 移動\n" \
	"L/R  = カーソ\ルの移動\n\n" \
	PB_SYM_PSP_CIRCLE   " = 入力\n" \
	PB_SYM_PSP_TRIANGLE " = スペース\n" \
	PB_SYM_PSP_SQUARE   " = 削除\n\n" \
	"SELECT = 文字レイアウト変更\n\n" \
	"START = 決定\n" \
	PB_SYM_PSP_CROSS "     = キャンセル"

#endif
