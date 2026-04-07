TARGET = LFS_PSP
OBJS = main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -D_PSP_FW_VERSION=600
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lraylib -lpng -lz -lglut -lGLU -lGL -lpspvfpu -lpsprtc -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspgum -lpspgu -lpspnet -lpspnet_apctl -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Live for Speed PSP Raylib

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
