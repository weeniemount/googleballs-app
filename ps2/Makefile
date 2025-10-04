EE_BIN = googleballs.elf
EE_OBJS = main.o

# Fixed library order and includes
EE_LIBS = -lgskit -ldmakit -lpadx -lm
EE_INCS += -I$(PS2SDK)/ports/include

all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal