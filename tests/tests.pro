TEMPLATE = subdirs

SUBDIRS = cpputils test-app cpp-template-utils cpp-db

exists(../../cpputils) {
	SUBREPOS_DIR=$${PWD}/../..
} else {
	SUBREPOS_DIR=$${PWD}
}

cpputils.subdir=$${SUBREPOS_DIR}/cpputils
cpp-template-utils.subdir=$${SUBREPOS_DIR}/cpp-template-utils
cpp-db.file=$${PWD}/../cpp-db.pro

#cpp-db.depends = cpputils cpp-template-utils

test-app.file=$${PWD}/test-app/test-app.pro
#test-app.depends = cpputils
