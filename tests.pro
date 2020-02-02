TEMPLATE = subdirs

SUBDIRS = tests cpputils cpp-template-utils cpp-db

exists(../cpputils) {
	SUBREPOS_DIR=$${PWD}/..
} else {
	SUBREPOS_DIR=$${PWD}
}

cpputils.subdir=$${SUBREPOS_DIR}/cpputils
cpp-template-utils.subdir=$${SUBREPOS_DIR}/cpp-template-utils
#cpp-db.subdir=$${PWD}
cpp-db.file=$${PWD}/cpp-db.pro

#cpp-db.depends = cpputils cpp-template-utils

tests.depends = cpp-db cpputils
