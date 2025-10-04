TARGET = googleballs
OBJS = main.o

INCDIR = 
CFLAGS = -O2 -G0 -Wall -D_PSP_FW_VERSION=600
CXXFLAGS = $(CFLAGS) -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lstdc++ -lSDL_image -lSDL -lpspirkeyb -ljpeg -lpng -lz \
       -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower \
       -lpspgu -lpspgum -lpspaudiolib -lpspaudio -lpsphttp -lpspssl \
       -lpspwlan -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl \
       -lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver \
       -lpsputility -lpspuser -lpspkernel -lpspdebug -lpspdisplay \
       -lpspge -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Google Balls PSP
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_PIC1 = PIC1.PNG

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak