AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = axkeys \
axftp axftpd \
axsh axshd \
axping axpingd \
axpipe \
examples/client examples/server \
examples/client_lockcheck \
examples/client_sign examples/server_sign

# Main binaries
axkeys_SOURCES=src/main.cc src/axkeys.cc src/axlib.cc src/crypto.cc
axping_SOURCES=src/main.cc src/axping.cc src/axlib.cc src/crypto.cc
axpingd_SOURCES=src/main.cc src/axpingd.cc src/axlib.cc src/crypto.cc
axftp_SOURCES=src/main.cc src/axftp.cc src/axlib.cc src/crypto.cc
axftpd_SOURCES=src/main.cc src/axftpd.cc src/axlib.cc src/crypto.cc
axsh_SOURCES=src/main.cc src/axsh.cc src/axlib.cc src/crypto.cc
axshd_SOURCES=src/main.cc src/axshd.cc src/axlib.cc src/crypto.cc
axpipe_SOURCES=src/main.cc src/axpipe.cc src/axlib.cc src/crypto.cc

# Examples
examples_client_SOURCES=examples/client.cc src/axlib.cc src/crypto.cc
examples_client_lockcheck_SOURCES=examples/client_lockcheck.cc src/axlib.cc src/crypto.cc
examples_server_SOURCES=examples/server.cc src/axlib.cc src/crypto.cc
examples_client_sign_SOURCES=examples/client_sign.cc src/axlib.cc src/crypto.cc
examples_server_sign_SOURCES=examples/server_sign.cc src/axlib.cc src/crypto.cc

mrproper: maintainer-clean
	rm -fr .deps Makefile.in aclocal.m4 compile config.h.in configure depcomp install-sh missing
