AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = axkeys axftp axftpd

axkeys_SOURCES=src/axkeys.cc src/axlib.cc src/crypto.cc
axftp_SOURCES=src/axftp.cc src/axlib.cc src/crypto.cc
axftpd_SOURCES=src/axftpd.cc src/axlib.cc src/crypto.cc

mrproper: maintainer-clean
	rm -fr .deps Makefile.in aclocal.m4 compile config.h.in configure depcomp install-sh missing
