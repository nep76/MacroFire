TARGET = macrofire
PSP_FW_VERSION = 390

BUILD_PRX = 1

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
# でも機能制限されすぎなので使わない。
# 無指定だとnewlibを使おうとしてくれる模様。
#USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードの方(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

CLASSG_LIBS = libs

INCDIR = $(CLASSG_LIBS)
CFLAGS = -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =

# newlibを使う場合、libpspuserをリンクしないとsceKernelFreeMemSizeのリンクに失敗する。
# libpspuserより先にlibpspkernelをリンクしないと、システムコントロール系のシンボルに
# ユーザモードの方をリンクしてしまい、カーネルモードスレッドを生成できない模様？
LIBS = -lpsprtc -lpspkernel -lc -lpspuser

OBJS = main.o \
       menu.o \
       hook.o \
       function/rapidfire.o \
       function/macromgr.o \
       function/macro.o \
       function/macroeditor.o \
       $(CLASSG_LIBS)/sstring.o \
       $(CLASSG_LIBS)/psp/blit.o \
       $(CLASSG_LIBS)/psp/memsce.o \
       $(CLASSG_LIBS)/psp/cmndlg.o \
       $(CLASSG_LIBS)/psp/cmndlg/getdigits.o \
       $(CLASSG_LIBS)/psp/cmndlg/getbuttons.o

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
