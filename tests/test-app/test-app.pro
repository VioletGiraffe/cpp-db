CONFIG += strict_c++ c++2a
QT = core

TEMPLATE = app
CONFIG += console

DESTDIR  = ../bin_tests/
OBJECTS_DIR = $${DESTDIR}/build/
MOC_DIR     = $${DESTDIR}/build/
UI_DIR      = $${DESTDIR}/build/
RCC_DIR     = $${DESTDIR}/build/

mac* | linux* | freebsd {
	CONFIG(release, debug|release):CONFIG *= Release optimize_full
	CONFIG(debug, debug|release):CONFIG *= Debug
}
contains(QT_ARCH, x86_64) {
	ARCHITECTURE = x64
} else {
	ARCHITECTURE = x86
}

Release:OUTPUT_DIR=bin/release/$${ARCHITECTURE}
Debug:OUTPUT_DIR=bin/debug/$${ARCHITECTURE}

win*{
	QMAKE_CXXFLAGS += /std:c++latest /permissive- /Zc:__cplusplus /Zc:char8_t

	QMAKE_CXXFLAGS += /MP /Zi /FS
	QMAKE_CXXFLAGS += /wd4251
	QMAKE_CXXFLAGS_WARN_ON = /W4
	DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX _SCL_SECURE_NO_WARNINGS

	QMAKE_LFLAGS += /DEBUG:FASTLINK

	Debug:QMAKE_LFLAGS += /INCREMENTAL
	Release:QMAKE_LFLAGS += /OPT:REF /OPT:ICF
}

linux*|mac*{
	QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-c++11-extensions -Wno-local-type-template-args -Wno-deprecated-register
	QMAKE_CXXFLAGS += -std=c++2a

	Release:DEFINES += NDEBUG=1
	Debug:DEFINES += _DEBUG
}

*g++*:QMAKE_CXXFLAGS += -fconcepts

INCLUDEPATH += \
	$${PWD}/../../src \
	$${PWD}/../cpp-template-utils \
	$${PWD}/../../../cpp-template-utils \
	$${PWD}/../cpputils \
	$${PWD}/../../../cpputils

SOURCES += tests_main.cpp \
	benchmarks/dbfilegaps_benchmarks.cpp \
	benchmarks/dbindex_benchmarks.cpp \
	dbfield_tests.cpp \
	dbfilegaps_tester.cpp \
	cpp-db_sanity_checks.cpp \
	dbfilegaps_load-store_test.cpp \
	dbfilegaps_operations_test.cpp \
	dbfilegaps_test.cpp \
	dbindex_test.cpp \
	dbindices_test.cpp \
	dbrecord_tests.cpp \
	dbstorage_tests.cpp \
	dbwal_tests.cpp \
	index_test_helpers.cpp

HEADERS += \
	dbfilegaps_tester.hpp

LIBS += -L$${PWD}/../$${OUTPUT_DIR} -L$${PWD}/../../../$${OUTPUT_DIR} -lcpputils
