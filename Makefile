TARGET = LFS_PSP
OBJS = main.o

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBS = -lpspgum -lpspgu -lpspdebug -lpspdisplay -lpspctrl -lpspsdk -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Live for Speed PSP
PSP_EBOOT_ID = LFS00001

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
