TOOLCHAIN  := $(OO_PS4_TOOLCHAIN)
PROJDIR    := balls
INTDIR     := build
CONTENT_ID := IV0000-GBAL00001_00-GOOGLEBALLS00000

PKG_FILES := eboot.bin sce_sys/param.sfo sce_sys/icon0.png \
             $(wildcard sce_module/libc.prx sce_module/libSceFios2.prx sce_sys/about/right.sprx) \
             $(wildcard sce_sys/pic0.png sce_sys/pic1.png sce_sys/snd0.at9)

CXX := clang++
LD  := ld.lld

LIBS := -lc -lc++ -lkernel -lSceVideoOut -lScePad -lSceUserService -lSceSystemService

CXXFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -O2 -std=c++17 -c \
            -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include \
            -isystem $(TOOLCHAIN)/include/c++/v1

LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr \
           -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

.PHONY: all pkg clean

all: pkg

$(INTDIR):
	mkdir -p $(INTDIR)

$(INTDIR)/main.o: main.cpp | $(INTDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(INTDIR)/$(PROJDIR).elf: $(INTDIR)/main.o
	$(LD) $^ -o $@ $(LDFLAGS)

eboot.bin: $(INTDIR)/$(PROJDIR).elf
	$(TOOLCHAIN)/bin/linux/create-fself -in=$< -out=$(INTDIR)/$(PROJDIR).oelf \
	    --eboot eboot.bin --paid 0x3800000000000011
	@if [ -f $(INTDIR)/eboot.bin ]; then mv -f $(INTDIR)/eboot.bin eboot.bin; fi

pkg: eboot.bin
	@test -f sce_module/libc.prx || echo "libc.prx missing"
	@test -f sce_module/libSceFios2.prx || echo "libSceFios2 missing"
	$(TOOLCHAIN)/bin/linux/create-gp4 -out $(PROJDIR).gp4 \
	    --content-id=$(CONTENT_ID) --files "$(PKG_FILES)"
	$(TOOLCHAIN)/bin/linux/PkgTool.Core pkg_build $(PROJDIR).gp4 .
	@echo "$(CONTENT_ID).pkg has been ballified"

clean:
	rm -rf $(INTDIR) eboot.bin $(PROJDIR).gp4 *.pkg