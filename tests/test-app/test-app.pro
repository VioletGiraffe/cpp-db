CONFIG += strict_c++ c++_latest # this is an intentional typo, c++_latest is not a valid specifier which leads qmake to NOT generate any -std=... flag at all, which is what I need

CONFIG -= qt

TEMPLATE = app
CONFIG += console

mac* | linux* | freebsd {
	CONFIG(release, debug|release):CONFIG *= Release optimize_full
	CONFIG(debug, debug|release):CONFIG *= Debug
}

Release:OUTPUT_DIR=release
Debug:OUTPUT_DIR=debug

DESTDIR  = ../bin/$${OUTPUT_DIR}/
OBJECTS_DIR = ../build/$${OUTPUT_DIR}
MOC_DIR     = ../build/$${OUTPUT_DIR}
UI_DIR      = ../build/$${OUTPUT_DIR}
RCC_DIR     = ../build/$${OUTPUT_DIR}

win*{
	QMAKE_CXXFLAGS += /std:c++latest /permissive- /Zc:__cplusplus /Zc:char8_t

	QMAKE_CXXFLAGS += /MP /FS /diagnostics:caret
	QMAKE_CXXFLAGS += /wd4251
	QMAKE_CXXFLAGS_WARN_ON = /W4
	DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS

	QMAKE_CXXFLAGS_DEBUG -= -Zi
	QMAKE_CXXFLAGS_DEBUG *= /ZI
	Debug:QMAKE_LFLAGS += /DEBUG:FASTLINK /INCREMENTAL

	Release:QMAKE_CXXFLAGS += /Zi

	Release:QMAKE_LFLAGS += /DEBUG:FULL /OPT:REF /OPT:ICF /INCREMENTAL /TIME
}

linux*|mac*{
	QMAKE_CXXFLAGS += -std=c++2b

	QMAKE_CXXFLAGS_WARN_ON = -Wall -Wextra -Wdelete-non-virtual-dtor -Werror=duplicated-cond -Werror=duplicated-branches -Warith-conversion -Warray-bounds -Wattributes -Wcast-align -Wcast-qual -Wconversion -Wdate-time -Wduplicated-branches -Wendif-labels -Werror=overflow -Werror=return-type -Werror=shift-count-overflow -Werror=sign-promo -Werror=undef -Wextra -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnull-dereference -Wpedantic -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-aliasing -Wstrict-aliasing=3 -Wuninitialized -Wunused-const-variable=2 -Wwrite-strings -Wlogical-op
	QMAKE_CXXFLAGS_WARN_ON += -Wno-missing-include-dirs -Wno-undef

	Release:DEFINES += NDEBUG=1
	Debug:DEFINES += _DEBUG
}

*g++*{
	QMAKE_CXXFLAGS += -fconcepts -fsanitize=thread -ggdb3 -fuse-ld=gold
	QMAKE_LFLAGS += -fsanitize=thread
}

INCLUDEPATH += \
	$${PWD}/../../src \
	$${PWD}/../cpp-template-utils \
	$${PWD}/../../../cpp-template-utils \
	$${PWD}/../cpputils \
	$${PWD}/../../../cpputils

SOURCES += tests_main.cpp \
#	benchmarks/dbfilegaps_benchmarks.cpp \
	benchmarks/dbindex_benchmarks.cpp \
	dbfield_tests.cpp \
#	dbfilegaps_tester.cpp \
	cpp-db_sanity_checks.cpp \
#	dbfilegaps_load-store_test.cpp \
#	dbfilegaps_operations_test.cpp \
#	dbfilegaps_test.cpp \
	dbindex_test.cpp \
	dbindices_test.cpp \
	dbops_test.cpp \
	dbrecord_tests.cpp \
	dbstorage_tests.cpp \
	dbwal_tests.cpp \
	index_test_helpers.cpp

HEADERS += \
	dbfilegaps_tester.hpp

LIBS += -L$${PWD}/../bin/$${OUTPUT_DIR} -L$${PWD}/../../../bin/$${OUTPUT_DIR} -L$${PWD}/../../../bin/$${OUTPUT_DIR}/x64 -lcpp-db -lthin_io -lcpputils
