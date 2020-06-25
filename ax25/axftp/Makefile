all: axftp axftpd axkeys
axkeys: axkeys.cc
	c++ -g -std=c++14 axkeys.cc -o axkeys -lsodium
axftp: axftp.cc axlib.cc crypto.cc
	c++ -g -std=c++14 axftp.cc axlib.cc crypto.cc -o axftp -lax25 -lpthread -lsodium
axftpd: axftpd.cc axlib.cc crypto.cc
	c++ -g -std=c++14 axftpd.cc axlib.cc crypto.cc -o axftpd -lax25 -lpthread -lsodium
client: client.cc axlib.cc axlib.h
	c++ -g -std=c++14 client.cc axlib.cc crypto.cc -o client -lax25 -lpthread -lsodium
server: server.cc axlib.cc axlib.h
	c++ -g -std=c++14 server.cc axlib.cc crypto.cc -o server -lax25 -lpthread -lsodium
