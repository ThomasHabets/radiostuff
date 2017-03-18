#!/usr/bin/python

import socket

buttons = {
    'cf9f9933fe64cff27ce679f3c9e7': 'power',
    'fccf99f33ce4cff27ce679f3c9e7': 'l-up',
    'cfc99ff33ce4cff27ce679f3c9e7': 'l-up-hold-long',
    'f3cf99f33e64cff27ce679f3c9e7': 'l-down',
    'fccf99f33e64cff27ce679f3c9e7': 'l-down-hold-short',
    'e67f99f33e64cff27ce679f3c9e7': 'l-down-hold-long',
    'fccf99e73ce4cff27ce679f3c9e7': 'r-up',
    'cfc99fe73ce4cff27ce679f3c9e7': 'r-up-hold',
    'f3cf99e73e64cff27ce679f3c9e7': 'r-down',
    'fccf99e73e64cff27ce679f3c9e7': 'r-down-hold-short',
    'e67f99e73e64cff27ce679f3c9e7': 'r-down-hold-long',
}

def main():
    host = '::'
    port = 12346
    sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    sock.bind((host, port))
    while True:
        data, addr = sock.recvfrom(1500)
        h = ''.join(['%02x' % ord(x) for x in data])
        t = buttons.get(h, h)
        if True or t != h:
            print buttons.get(h, h)

if __name__ == '__main__':
    main()
