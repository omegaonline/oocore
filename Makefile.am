ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libltdl include src test

gitbundle:
	git bundle create omega.git master

reallyclean: maintainer-clean
	rm -f Makefile
	rm -f Makefile.in
	rm -f aclocal.m4
	rm -f config.log
	rm -f config.status
	rm -f configure
	rm -f include/Makefile
	rm -f include/Makefile.in
	rm -f libtool
	rm -f m4/argz.m4
	rm -f m4/libtool.m4
	rm -f m4/ltdl.m4
	rm -f m4/ltoptions.m4
	rm -f m4/ltsugar.m4
	rm -f m4/ltversion.m4
	rm -f m4/lt~obsolete.m4
	rm -f src/Makefile
	rm -f src/Makefile.in
	rm -f src/OOBase/Makefile
	rm -f src/OOBase/Makefile.in
	rm -f src/OOBase/autoheader_dummy.h.in~
	rm -f src/OOBase/stamp-h1
	rm -f src/OOBase/stamp-h2
	rm -f src/OOCore/Makefile
	rm -f src/OOCore/Makefile.in
	rm -f src/OORegister/Makefile
	rm -f src/OORegister/Makefile.in
	rm -f src/OOServer/Makefile
	rm -f src/OOServer/Makefile.in
	rm -f test/CoreTests/Makefile
	rm -f test/CoreTests/Makefile.in
	rm -f test/CoreTests/TestLibrary/Makefile
	rm -f test/CoreTests/TestLibrary/Makefile.in
	rm -f test/CoreTests/TestProcess/Makefile
	rm -f test/CoreTests/TestProcess/Makefile.in
	rm -f test/Makefile
	rm -f test/Makefile.in
