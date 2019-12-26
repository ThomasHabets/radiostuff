#!/usr/bin/env python3
import time
import sys
import codecs
import serial
import argparse 
from lostik import send_cmd, read_line

parser = argparse.ArgumentParser(description='LoRa Radio mode receiver.')
parser.add_argument('port', help="Serial port descriptor")
args = parser.parse_args()

def setup(ser):
    ver = send_cmd(ser, "sys get ver")
    print("Chip version: <%s>" % ver)
    ok = lambda x: x == "ok"
    for cmd, checker in (
            ("mac pause", lambda x: int(x)),
            ("radio set pwr 10", ok),
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
    while True:
        reply = send_cmd(ser, "radio rx 0")
        if reply == "busy":
            raise Exception("bad state! <radio rx> command received <busy>")

        # Wait for data.
        radio = read_line(ser)
        if radio == "radio_err":
            # No message at this point.
            continue
        if not radio.startswith('radio_rx '):
            raise Exception("unexpected reply to 'radio rx 0': <%s>" % radio)
        radio = radio[len('radio_rx '):].strip()
        dec = codecs.decode(radio, 'hex').decode('ascii')
        send_cmd(ser, "sys set pindig GPIO10 1", "ok")
        send_cmd(ser, "sys set pindig GPIO10 0", "ok")
        print("Reply> %s" % dec)
    
def main():
    ser = serial.Serial(args.port, baudrate=57600)
    mainloop(ser)

if __name__ == '__main__':
    main()

