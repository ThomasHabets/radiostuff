#!/usr/bin/env python3                                                                                                                                                                                             
import time
import sys
import serial
import argparse
from lostik import send_cmd, read_line

parser = argparse.ArgumentParser(description='LoRa Radio mode beacon.')
parser.add_argument('port', help="Serial port descriptor")
parser.add_argument('message', help="Beacon message")
args = parser.parse_args()

def to_hex(s):
    return ''.join([hex(ord(c))[2:].zfill(2) for c in s])
                    
def setup(ser):
    ver = send_cmd(ser, "sys get ver")                                                                                                                                                                            
    print("Chip version: <%s>" % ver)                                                                                                                                                                             
    ok = lambda x: x == "ok"
    anything = lambda x: True
    for cmd, checker in (
            ("mac pause", lambda x: int(x)),
            ("radio set pwr 10", ok),
            ("radio get mod", anything),
            ("radio get freq", anything),
            ("radio get sf", anything),
            ("radio set wdt 10000", ok),
            ("sys set pindig GPIO10 0", ok),
            ("sys set pindig GPIO11 0", ok),
    ):
        reply = send_cmd(ser, cmd)
        print("Reply to <%s>: <%s>" % (cmd, reply))
        if not checker(reply):
            raise Exception("bad reply for command <%s>: <%s>" % (cmd, reply))

def mainloop(ser):
    setup(ser)
    print("Main loop starting…")
    count = 0
    while True:
        time.sleep(1)
        count += 1
        
        send_cmd(ser, "sys set pindig GPIO11 1", "ok")
        cmd = "radio tx %s" % to_hex('%s %d' % (args.message, count))
        reply = send_cmd(ser, cmd)
        if reply != "ok":
            raise Exception("failed to queue sending message. Command <%s>, reply <%s>" % (cmd, reply))
        ok = read_line(ser)
        send_cmd(ser, "sys set pindig GPIO11 0", "ok")
        if ok != "radio_tx_ok":
            print("TX failed. Command: <%s>, reply <%s>" % (cmd, reply))
            continue

        print("Reply> %s" % reply)
        

        
def main():
    ser = serial.Serial(args.port, baudrate=57600)
    mainloop(ser)

if __name__ == '__main__':
    main()
