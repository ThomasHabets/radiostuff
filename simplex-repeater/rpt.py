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
indev='hw:1,0'
outdev='hw:1,0'
skip=3

def fix(d):
    """Turn mono data into stereo"""
    line=d
    n=2
    return ''.join([line[i:i+n]*2 for i in range(0, len(line), n)])
    shorts = struct.unpack('<' + 'h' * (len(d)/2), d)
    dbl = reduce(lambda x,y: x+y, zip(shorts, shorts))
    return struct.pack('<' + 'h' * len(d), *dbl)

def record(inp):
    triggered = False
    alldata = []
    silent = 0

    # Skip a bit before starting.
    print '... skipping %ds' % (skip)
    for _ in range(int(skip*float(rate)/periodsize)):
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

        alldata.append(fix(data))


def main():
    inp = audio.PCM(audio.PCM_CAPTURE,card=indev)
    inp.setchannels(1)
    inp.setrate(rate)
    #inp.setformat(audioformat)
    inp.setperiodsize(periodsize)

    out = audio.PCM(audio.PCM_PLAYBACK,card=outdev)
    out.setchannels(2)
    out.setrate(rate)
    #out.setformat(audioformat)
    out.setperiodsize(periodsize)

    while True:
        print "recording..."
        st = time.time()
        rec = record(inp)
        print '... took %ds and gave %d data' % (time.time()-st, len(rec))
        print '... %d data should be %.2fs' % (len(rec), len(rec)*periodsize/44100)

        if False:
            st = time.time()
            fixed = []
            print 'transforming...'
            for piece in rec:
                fixed.append(fix(piece))
            print '... took', (time.time()-st)
        else:
            fixed = rec    
            
        st = time.time()
        print "playback..."
        for piece in fixed:
            out.write(piece)
        print '... took', (time.time()-st)

main()
