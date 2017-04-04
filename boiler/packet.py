#!/usr/bin/python

import numpy as np
import sys
import time

def show_packet(packet):
    if len(packet) == 0:
        return
    prefix = [0,1,0,1, 0,1,0,1, 0,1,0,0, 0,1,0,1]
    payload = None
    for i in range(len(packet)):
        if packet[i:i+len(prefix)] == prefix:
            payload = packet[i:]
            break
    if payload:
        print time.time(), ' '.join([hex(x) for x in np.packbits(np.array(payload))])
    else:
        print packet

def main():
    packet = []
    while True:
        b = sys.stdin.read(1)
        if len(b) == 0:
            sys.exit(0)
        b = ord(b)
        if b == 0:
            show_packet(packet)
            packet = []
            continue
        if b < 128:
            packet.append(1)
        else:
            packet.append(0)


main()
