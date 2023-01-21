TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = cpp-template-utils cpputils cpp-db test-app

exists(../../cpputils) {
	SUBREPOS_DIR=$${PWD}/../..
} else {
	SUBREPOS_DIR=$${PWD}
}

cpp-template-utils.subdir=$${SUBREPOS_DIR}/cpp-template-utils

cpputils.file=$${SUBREPOS_DIR}/cpputils/cpputils.pro

cpp-db.file=$${PWD}/../cpp-db.pro
cpp-db.depends = cpputils

test-app.file=$${PWD}/test-app/test-app.pro
test-app.depends = cpp-db
