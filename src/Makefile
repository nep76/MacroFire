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

CLASSG_LIBS = ./include

INCDIR   = $(CLASSG_LIBS)
CFLAGS   = -O2 -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS  = $(CFLAGS)
LIBDIR   = 
LDFLAGS  =

# newlibを使う場合、libpspuserをリンクしないとsceKernelFreeMemSizeのリンクに失敗する。
# libpspuserより先にlibpspkernelをリンクしないと、システムコントロール系のシンボルに
# ユーザモードの方をリンクしてしまい、カーネルモードスレッドを生成できない模様？
LIBS = -lpsppower_driver -lpspge -lpsprtc -lpspkernel -lc -lm -lpspuser

OBJS = exports.o \
       sceDmac.o \
       main.o \
       menu.o \
       analogtune.o \
       rapidfire.o \
       function/rapidfire.o \
       function/macromgr.o \
       function/macro.o \
       function/macroeditor.o \
       $(CLASSG_LIBS)/utils/strutil.o \
       $(CLASSG_LIBS)/utils/inimgr.o \
       $(CLASSG_LIBS)/psp/hook.o \
       $(CLASSG_LIBS)/psp/ctrlpad.o \
       $(CLASSG_LIBS)/psp/gb.o \
       $(CLASSG_LIBS)/psp/memsce.o \
       $(CLASSG_LIBS)/psp/pathexpand.o \
       $(CLASSG_LIBS)/psp/fileh.o \
       $(CLASSG_LIBS)/psp/dirh.o \
       $(CLASSG_LIBS)/psp/cmndlg.o \
       $(CLASSG_LIBS)/psp/cmndlg/message.o \
       $(CLASSG_LIBS)/psp/cmndlg/getnumbers.o \
       $(CLASSG_LIBS)/psp/cmndlg/getbuttons.o \
       $(CLASSG_LIBS)/psp/cmndlg/getfilename.o \
       $(CLASSG_LIBS)/psp/cmndlg/sosk.o

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
