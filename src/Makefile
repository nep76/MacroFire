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

CLASSG_LIBS = include

INCDIR   = $(CLASSG_LIBS)
CFLAGS   = -O2 -G 0 -Wall -fno-strict-aliasing# -g -DDEBUG=1 -DMEMORY_DEBUG
ASFLAGS  = $(CFLAGS)
LIBDIR   = 
LDFLAGS  = 

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
       $(CLASSG_LIBS)/utils/strutil.o \
       $(CLASSG_LIBS)/utils/inimgr.o \
       $(CLASSG_LIBS)/psp/memory.o \
       $(CLASSG_LIBS)/psp/heap.o \
       $(CLASSG_LIBS)/psp/dmem.o \
       $(CLASSG_LIBS)/psp/gb.o \
       $(CLASSG_LIBS)/psp/pathexpand.o \
       $(CLASSG_LIBS)/psp/dirh.o \
       $(CLASSG_LIBS)/psp/fiomgr.o \
       $(CLASSG_LIBS)/psp/ovmsg.o \
       $(CLASSG_LIBS)/psp/hook.o \
       $(CLASSG_LIBS)/psp/padctrl.o \
       $(CLASSG_LIBS)/psp/padutil.o \
       $(CLASSG_LIBS)/psp/cdialog.o \
       $(CLASSG_LIBS)/psp/cdialog/message.o \
       $(CLASSG_LIBS)/psp/cdialog/sosk.o \
       $(CLASSG_LIBS)/psp/cdialog/numedit.o \
       $(CLASSG_LIBS)/psp/cdialog/getfilename.o \
       $(CLASSG_LIBS)/psp/cdialog/detectbuttons.o \

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
