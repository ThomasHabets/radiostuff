#!/usr/bin/python
import alsaaudio as audio
import time
import os
import struct
import sys

audioformat=audio.PCM_FORMAT_S16_LE
# second arg to PCM can be ,audio.PCM_NONBLOCK
periodsize=1024
rate=44100

def fix(d):
    """Turn mono data into stereo"""
    shorts = struct.unpack('<' + 'h' * (len(d)/2), d)
    dbl = reduce(lambda x,y: x+y, zip(shorts, shorts))
    return struct.pack('<' + 'h' * len(d), *dbl)

def record(inp):
    triggered = False
    alldata = []
    silent = 0
    # Skip 2 seconds.
    print '... skipping 2s'
    for _ in range(2 * int(float(rate)/periodsize)):
        inp.read()
    print '... ok, listening'
    while True:
        l,data = inp.read()
        if len(data) == 0:
            continue
        if len(data) & 1 != 0:
            continue
        shorts = struct.unpack('<' + 'h' * (len(data)/2), data)
        avg = sum([abs(x) for x in shorts])/len(shorts)
        if not triggered and avg < 2000:
            continue
        elif not triggered:
            print '... triggered with avg %f' % (avg)
        triggered = True

        if avg < 2000:
            silent += 1
        if silent > 100:
            return alldata

        alldata.append(data)


def main():
    inp = audio.PCM(audio.PCM_CAPTURE,card='hw:2,0')
    inp.setchannels(1)
    inp.setrate(rate)
    #inp.setformat(audioformat)
    inp.setperiodsize(periodsize)

    out = audio.PCM(audio.PCM_PLAYBACK,card='hw:1,0')
    out.setchannels(2)
    out.setrate(rate)
    #out.setformat(audioformat)
    out.setperiodsize(periodsize)

    while True:
        print "recording..."
        st = time.time()
        rec = record(inp)
        print '... took %ds and gave %d data' % (time.time()-st, len(rec))

        st = time.time()
        print "playback..."
        for piece in rec:
            out.write(fix(piece))
        print '... took', (time.time()-st)

main()
