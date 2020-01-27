CONFIG += c++1z strict_c++
CONFIG -= qt

TEMPLATE = app
CONFIG += console

win*{
	QMAKE_CXXFLAGS += /std:c++17 /permissive- /Zc:__cplusplus

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

	Release:DEFINES += NDEBUG=1
	Debug:DEFINES += _DEBUG
}

INCLUDEPATH += \
	$${PWD}/../src/ \
	$${PWD}/../cpp-template-utils/ \
	$${PWD}/../../cpp-template-utils/

SOURCES += tests_main.cpp

HEADERS += \
	../src/cpp-db.hpp \
	../src/dbfield.hpp \
	../src/dbfilegaps.hpp \
	../src/dbindex.hpp \
	../src/dbindices.hpp \
	../src/dbrecord.hpp \
	../src/dbstorage.hpp \
	../src/index_helpers.hpp \
	../src/storage_io.hpp
