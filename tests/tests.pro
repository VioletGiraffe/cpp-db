TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = cpp-template-utils thin_io cpputils cpp-db test-app

exists(../../cpputils) {
	SUBREPOS_DIR=$${PWD}/../..
} else {
	SUBREPOS_DIR=$${PWD}
}

cpp-template-utils.subdir=$${SUBREPOS_DIR}/cpp-template-utils

thin_io.file=$${SUBREPOS_DIR}/thin_io/thin_io.pro

cpputils.file=$${SUBREPOS_DIR}/cpputils/cpputils.pro

cpp-db.file=$${PWD}/../cpp-db.pro
cpp-db.depends = cpputils thin_io

test-app.file=$${PWD}/test-app/test-app.pro
test-app.depends = cpp-db
