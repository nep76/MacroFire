#======================================
# MacroFireをビルドするためのMakefile.
# 
#   使用可能なmakeターゲット:
#
#   all(default) - MacroFire本体を作成する
#   clean        - MacroFire本体とオブジェクトファイルを削除
#   macrofire    - allの別名
#
#   libs         - MacroFireをビルドするのに必要なライブラリを作成する
#   libsclean    - ライブラリ本体とオブジェクトファイルを削除
#
#   everything   - libs + macrofire
#   allclean     - clean + libsclean
#
#--------------------------------------
# 設定可能なオプション
#
# 日本語フォントと日本語表示サポート:
#     WITH_JAPANESE=1
#
# 日本語フォントのJIS第二水準までサポート:
#     WITH_JAPANESE=1 WITH_JIS_LEVEL2=1
#
# * 日本語フォントを使用する場合は、
# * MacroFire本体だけでなくライブラリも同じオプションをつけてビルドする必要があります。
# * そうでなければダイアログの表示がおかしくなるでしょう。
# *   make WITH_JAPANESE=1 allclean everything
#
#--------------------------------------
# MacroFire本体にのみ影響のあるオプション
#
# 例外ハンドラを設定する:
#     WITH_EXCEPTION_HANDLER=1
#
# PSPLINK用にデバッグメッセージをprintf出力する:
#     WITH_PSPLINK_DEBUG=1
#
# メモリの割り付け/解放をPSPLINKへ出力 (使用するスタックメモリが増えることに注意)
#     WITH_PSPLINK_DEBUG=1 WITH_MEMORY_DEBUG=1
#
#======================================

TARGET = macrofire
PSP_FW_VERSION = 380

# PRXを作成する
BUILD_PRX = 1

# エクスポート関数定義ファイル
PRX_EXPORTS = exports.exp

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードのライブラリ(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

BASEPATH = .

INCDIR  = $(BASEPATH)/include
LIBDIR  = $(BASEPATH)/lib

CFLAGS      = -Os -G0 -Wall -fno-strict-aliasing
ASFLAGS     = $(CFLAGS)
LDFLAGS     = 
LIBS        = -lpsppower_driver -lpsphprm_driver -lpspge -lpsprtc -lpsputility -lm
CLASSG_LIBS = -lcdialog -lovmsg -linimgr -ldirh -lfiomgr -lpadctrl -lpadutil -lpb -ldmem -lheap -lmemory -lmakepath -lstrutil -lhook

OBJS = \
	exports.o \
	sceDmac.o \
	sceUtility.o \
	mfmain.o \
	mfanalogstick.o \
	mfrapidfire.o \
	mfctrl.o \
	mfdialog.o \
	mfmenu.o \
	functions/remap.o \
	functions/rapidfire.o \
	functions/macromgr.o \
	functions/macro.o \
	functions/macroeditor.o 

LIBS := $(CLASSG_LIBS) $(LIBS)

ifdef WITH_PSPLINK_DEBUG
	CFLAGS += -fno-builtin-printf -g -DDEBUG=1
endif

ifdef WITH_MEMORY_DEBUG
	CFLAGS += -DMEMORY_DEBUG
endif

ifdef WITH_EXCEPTION_HANDLER
	CFLAGS += -DMF_WITH_EXCEPTION_HANDLER -g
endif

ifdef WITH_JAPANESE
	CFLAGS += -DPB_SJIS_SUPPORT
	ifdef WITH_JIS_LEVEL2
		CFLAGS += -DPB_JIS_LEVEL2
	endif
endif

ifeq ($(USE_KERNEL_LIBC),1)
    CFLAGS += -DPSP_USE_KERNEL_LIBC
	OBJS += sceSystemMemoryManager.o
	LIBS += -lsysclib
else
	LIBS += -lpspkernel -lc -lpspuser
endif

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

everything: libs $(TARGET)

allclean: libsclean clean

$(TARGET): all

libs:
	@$(MAKE) -C $(BASEPATH)/include

libsclean:
	@$(MAKE) -C $(BASEPATH)/include clean
