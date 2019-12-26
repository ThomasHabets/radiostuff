#!/usr/bin/env python3

def send_cmd(ser, cmd, checker=None):
    print("SEND> %s" % cmd)
    ser.write((cmd + "\r\n").encode('utf-8'))
    ret = read_line(ser)
    if checker is not None:
        if ret != checker:
            raise Exception("bad reply to <%s>: Want <%s>, got <%s>" % (cmd, checker, ret))
    return ret

def read_line(ser):
    reply = ser.readline()
    return reply.strip().decode('utf-8')
