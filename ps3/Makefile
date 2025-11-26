.SUFFIXES:

ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

include $(PSL1GHT)/ppu_rules

TARGET		:=	googleballs
BUILD		:=	build
SOURCES		:=	.
DATA		:=	data
INCLUDES	:=	include
PKGFILES	:=	$(CURDIR)/pkg

TITLE		:=	Google Balls
APPID		:=	GBAL00001
CONTENTID	:=	UP0001-$(APPID)_00-0000000000000000

CFLAGS		+= -O2 -Wall -mcpu=cell $(MACHDEP) $(INCLUDE)
CXXFLAGS	+= $(CFLAGS)

LDFLAGS		=	$(MACHDEP) -Wl,-Map,$(notdir $@).map

# Add tiny3d library
LIBS		:=	-ltiny3d -lsysutil -lio -lrt -llv2

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

export BUILDDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

export BINFILES	:=	$(BINFILES)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					$(LIBPSL1GHT_INC) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					$(LIBPSL1GHT_LIB)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).pkg

run: $(BUILD)
	ps3load $(OUTPUT).self

pkg: $(BUILD)
	@echo Creating PKG...
	@mkdir -p $(PKGFILES)/USRDIR
	@cp $(OUTPUT).self $(PKGFILES)/USRDIR/EBOOT.BIN
	@$(FSELF) -n $(PKGFILES)/USRDIR/EBOOT.BIN
	@sfo.py --title "$(TITLE)" --appid "$(APPID)" -f $(PKGFILES)/PARAM.SFO
	@pkg.py --contentid $(CONTENTID) $(PKGFILES)/ $(OUTPUT).pkg
	@echo "Package $(OUTPUT).pkg created."

else

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif