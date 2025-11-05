## Haiku Generic Makefile ##

NAME = GoogleBalls
TYPE = APP
APP_MIME_SIG = application/x-vnd.GoogleBalls

SRCS = main.cpp

RDEFS = GoogleBalls.rdef

LIBS = be tracker translation

OPTIMIZE := FULL

WARNINGS = ALL

SYMBOLS := FALSE

DEBUGGER := FALSE

# Use static linking
LDFLAGS += -static-libgcc -static-libstdc++

# Include system directories
SYSTEM_INCLUDE_PATHS = /boot/system/develop/headers/be \
					   /boot/system/develop/headers/cpp \
					   /boot/system/develop/headers/posix

# Library paths for static linking
LIBPATHS = /boot/system/develop/lib

## Include the Makefile-Engine
DEVEL_DIRECTORY := /boot/system/develop
include $(DEVEL_DIRECTORY)/etc/makefile-engine