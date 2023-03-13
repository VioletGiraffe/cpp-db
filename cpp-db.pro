TEMPLATE = lib

CONFIG += staticlib
QT = core

CONFIG += strict_c++ c++2a
CONFIG -= flat

mac* | linux*{
	CONFIG(release, debug|release):CONFIG *= Release optimize_full
	CONFIG(debug, debug|release):CONFIG *= Debug
}
contains(QT_ARCH, x86_64) {
	ARCHITECTURE = x64
} else {
	ARCHITECTURE = x86
}

Release:OUTPUT_DIR=release/$${ARCHITECTURE}
Debug:OUTPUT_DIR=debug/$${ARCHITECTURE}

DESTDIR  = ../bin/$${OUTPUT_DIR}/
OBJECTS_DIR = ../build/$${OUTPUT_DIR}/$${TARGET}
MOC_DIR     = ../build/$${OUTPUT_DIR}/$${TARGET}
UI_DIR      = ../build/$${OUTPUT_DIR}/$${TARGET}
RCC_DIR     = ../build/$${OUTPUT_DIR}/$${TARGET}

win*{
	QMAKE_CXXFLAGS += /MP /Zi /FS
	QMAKE_CXXFLAGS += /std:c++latest /permissive- /Zc:__cplusplus /Zc:char8_t
	DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS
	QMAKE_CXXFLAGS += /wd4251
	QMAKE_CXXFLAGS_WARN_ON = /W4

	QMAKE_LFLAGS += /DEBUG:FASTLINK

	Debug:QMAKE_LFLAGS += /INCREMENTAL
	Release:QMAKE_LFLAGS += /OPT:REF /OPT:ICF
}

linux*|mac*{
	QMAKE_CXXFLAGS += -pedantic-errors -std=c++2a
	QMAKE_CFLAGS += -pedantic-errors

	QMAKE_CXXFLAGS_WARN_ON = -Wall -Wextra -Wdelete-non-virtual-dtor -Werror=duplicated-cond -Werror=duplicated-branches -Warith-conversion -Warray-bounds -Wattributes -Wcast-align -Wcast-qual -Wconversion -Wdate-time -Wduplicated-branches -Wendif-labels -Werror=overflow -Werror=return-type -Werror=shift-count-overflow -Werror=sign-promo -Werror=undef -Wextra -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnull-dereference -Wpedantic -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-aliasing -Wstrict-aliasing=3 -Wuninitialized -Wunused-const-variable=2 -Wwrite-strings -Wlogical-op
	QMAKE_CXXFLAGS_WARN_ON += -Wno-missing-include-dirs -Wno-undef

	Release:DEFINES += NDEBUG=1
	Debug:DEFINES += _DEBUG
}

*g++*: QMAKE_CXXFLAGS += -fconcepts -fuse-ld=gold

INCLUDEPATH += \
	../cpputils \
	../cpp-template-utils

HEADERS += $$files(src/*.hpp, true)

SOURCES += \
	src/utils/dbutilities.cpp
