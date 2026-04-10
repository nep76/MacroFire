# 例外ハンドラを設定するデバッグ用MacroFireをビルドするには
#     make WITH_EXCEPTION_HANDLER=1
#

TARGET = macrofire
PSP_FW_VERSION = 380

# PRXを作成する
BUILD_PRX = 1

# エクスポート定義ファイル
PRX_EXPORTS = exports.exp

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
# でも機能制限されすぎなので使わない。
# 無指定だとnewlibを使おうとしてくれる模様。
#USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードの方(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

DIR_CLASSG = include

INCDIR   = $(DIR_CLASSG)
CFLAGS   = -O2 -G 0 -Wall -fno-strict-aliasing -fno-builtin-printf# -g -DDEBUG=1 -DMEMORY_DEBUG
ASFLAGS  = $(CFLAGS)
LIBDIR   = 
LDFLAGS  = 
DEFINES  = 

# newlibを使う場合、libpspuserをリンクしないとsceKernelFreeMemSizeのリンクに失敗する。
# libpspuserより先にlibpspkernelをリンクしないと、システムコントロール系のシンボルに
# ユーザモードの方をリンクしてしまい、カーネルモードスレッドを生成できない模様？
LIBS = -lpsppower_driver -lpsphprm_driver -lpspge -lpsprtc -lpspkernel -lc -lm -lpspuser

OBJS = exports.o \
       sceDmac.o \
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
       functions/macroeditor.o \
       $(DIR_CLASSG)/utils/strutil.o \
       $(DIR_CLASSG)/utils/inimgr.o \
       $(DIR_CLASSG)/psp/hook.o \
       $(DIR_CLASSG)/psp/memory.o \
       $(DIR_CLASSG)/psp/heap.o \
       $(DIR_CLASSG)/psp/dmem.o \
       $(DIR_CLASSG)/psp/gb.o \
       $(DIR_CLASSG)/psp/pathexpand.o \
       $(DIR_CLASSG)/psp/dirh.o \
       $(DIR_CLASSG)/psp/fiomgr.o \
       $(DIR_CLASSG)/psp/ovmsg.o \
       $(DIR_CLASSG)/psp/padctrl.o \
       $(DIR_CLASSG)/psp/padutil.o \
       $(DIR_CLASSG)/psp/cdialog.o \
       $(DIR_CLASSG)/psp/cdialog/message.o \
       $(DIR_CLASSG)/psp/cdialog/sosk.o \
       $(DIR_CLASSG)/psp/cdialog/numedit.o \
       $(DIR_CLASSG)/psp/cdialog/getfilename.o \
       $(DIR_CLASSG)/psp/cdialog/detectbuttons.o \

ifdef WITH_EXCEPTION_HANDLER
	CFLAGS += -DMF_WITH_EXCEPTION_HANDLER -g
endif

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
