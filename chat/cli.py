#!/usr/bin/python
import socket
import sys
import threading
import readline

def rcv(sock):
    while True:
        data, address = sock.recvfrom(4096)
        print ['%02x' % ord(x) for x in data], data
        readline.redisplay()


def main():
    # Start receiver.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('localhost', 52002))
    thread = threading.Thread(target=rcv, args=(sock,))
    thread.daemon = True
    thread.start()

    # Start sender.
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    dst = ('localhost', 52001)
    print 'Start chatting'
    while True:
        line = raw_input('> ')
        sock.sendto(line, dst)
        print "sent that"
if __name__ == '__main__':
    try:
        main()
    except (KeyboardInterrupt, EOFError) as e:
        print "exit"
        sys.exit(1)
