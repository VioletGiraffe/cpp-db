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
	DEFINES += WIN32_LEAN_AND_MEAN NOMINMAX
	QMAKE_CXXFLAGS += /wd4251
	QMAKE_CXXFLAGS_WARN_ON = /W4

	QMAKE_LFLAGS += /DEBUG:FASTLINK

	Debug:QMAKE_LFLAGS += /INCREMENTAL
	Release:QMAKE_LFLAGS += /OPT:REF /OPT:ICF
}

linux*|mac*{
	QMAKE_CXXFLAGS += -pedantic-errors -std=c++2a
	QMAKE_CFLAGS += -pedantic-errors
	QMAKE_CXXFLAGS_WARN_ON = -Wall

	Release:DEFINES += NDEBUG=1
	Debug:DEFINES += _DEBUG
}

*g++*: QMAKE_CXXFLAGS += -fconcepts

INCLUDEPATH += \
	../cpputils \
	../cpp-template-utils

HEADERS += \
	src/WAL/operation_completion_marker.hpp \
	src/cpp-db.hpp \
	src/cppDb_compile_time_sanity_checks.hpp \
	src/dbfield_size_helpers.hpp \
	src/dbops.hpp \
	src/dbrecord.hpp \
	src/dbschema.hpp \
	src/dbwal.hpp \
	src/index/dbindex.hpp \
	src/index/dbindices.hpp \
	src/index/index_persistence.hpp \
	src/index_helpers.hpp \
	src/dbfield.hpp \
	src/dbstorage.hpp \
	src/serialization/dbrecord-serializer.hpp \
	src/storage/io_with_hashing.hpp \
	src/storage/storage_helpers.hpp \
	src/storage/storage_io_interface.hpp \
	src/storage/storage_qt.hpp \
	src/storage/storage_static_buffer.hpp \
	src/utils/dbutilities.hpp \
	src/WAL/wal_serializer.hpp

SOURCES += \
	src/utils/dbutilities.cpp
