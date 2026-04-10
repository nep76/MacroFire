#======================================
# 例外ハンドラを設定する:
#     make WITH_EXCEPTION_HANDLER=1
#
# PSPLINK用にデバッグメッセージをprintf出力する:
#     make WITH_PSPLINK_DEBUG=1
#
# 日本語フォントと日本語表示サポート:
#     make WITH_JAPANESE=1
#
# 日本語フォントのJIS第二水準までサポート:
#     make WITH_JAPANESE=1 WITH_JIS_LEVEL2=1
#
#======================================

TARGET = macrofire
PSP_FW_VERSION = 380

# PRXを作成する
BUILD_PRX = 1

# エクスポート関数定義ファイル
PRX_EXPORTS = exports.exp

# 機能が制限されているが軽量なカーネルlibc(sysclib)。
# printf系関数もないが、prntという関数を提供してくれるため、
# 自分で再実装するのは容易。
USE_KERNEL_LIBC = 1

# カーネルモードPRXを作る場合は推奨されているようだ。
# これを指定するといくつかのライブラリは
# 優先的にカーネルモードのライブラリ(psp***_driver)をリンクしようとするみたい。
USE_KERNEL_LIBS = 1

DIR_CLASSG = include

INCDIR   = $(DIR_CLASSG)
CFLAGS   = -Os -G 0 -Wall -fno-strict-aliasing
ASFLAGS  = $(CFLAGS)
LIBDIR   = 
LDFLAGS  = 
LIBS     = -lpsppower_driver -lpsphprm_driver -lpspge -lpsprtc -lm

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
       $(DIR_CLASSG)/psp/utils/inimgr.o \
       $(DIR_CLASSG)/psp/hook.o \
       $(DIR_CLASSG)/psp/memory.o \
       $(DIR_CLASSG)/psp/heap.o \
       $(DIR_CLASSG)/psp/dmem.o \
       $(DIR_CLASSG)/psp/pb.o \
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

ifdef WITH_PSPLINK_DEBUG
	CFLAGS += -fno-builtin-printf -g -DDEBUG=1 -DMEMORY_DEBUG
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
	OBJS += sceSystemMemoryManager.o \
            $(DIR_CLASSG)/psp/sysclib/sysclib.o
else
	LIBS += -lpspkernel -lc -lpspuser
endif

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
