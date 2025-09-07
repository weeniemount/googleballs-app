TARGET = googleballs
OBJS = main.o

INCDIR = 
CFLAGS = -O2 -G0 -Wall -D_PSP_FW_VERSION=500
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lSDL_image -lSDL -lGL -lGLU -lglut -ljpeg -lpng -lz -lm -lpsphprm -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspgum -lpspaudiolib -lpspaudio -lpsphttp -lpspssl -lpspwlan -lpspnet_adhocmatching -lpspnet_adhoc -lpspnet_adhocctl -lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver -lpsputility -lpspuser -lpspkernel

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Google Balls PSP
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_PIC1 = PIC1.PNG

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak