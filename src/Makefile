TARGET = macrofire
PSP_FW_VERSION = 390

BUILD_PRX = 1

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

CLASSG_LIBS = ./libs

INCDIR = $(CLASSG_LIBS)
CFLAGS = -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS = -mno-crt0 -nostartfiles
LIBS =

OBJS = main.o \
       menu.o \
       hook.o \
       function/rapidfire.o \
       function/macro.o \
       $(CLASSG_LIBS)/sstring.o \
       $(CLASSG_LIBS)/psp/blit.o \
       $(CLASSG_LIBS)/psp/memsce.o \

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak
