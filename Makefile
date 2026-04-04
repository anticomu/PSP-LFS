TARGET = LFS_PSP
OBJS = main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspgu -lpspdebug -lpspdisplay -lpspctrl -lpspsdk -lpspnet_adhoc -lpspnet_adhocctl -lpsprtc -lpsppower -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Live for Speed PSP
# PSP_EBOOT_ICON = icon0.png
# PSP_EBOOT_PIC1 = pic1.png

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
