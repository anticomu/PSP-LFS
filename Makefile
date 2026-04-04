TARGET = LFS_PSP
OBJS = main.o
LIBS = -lpspgum -lpspgu -lpspdebug -lpspdisplay -lpspctrl -lpspsdk -lpspnet_adhoc -lpspnet_adhocctl -lpsprtc -lpsppower -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Live for Speed PSP
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
