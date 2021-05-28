#!/usr/bin/python3
#
# composite -blend 50 foo.png Maidenhead_Locator_Map.png blah.png
import re
import sys
import png
import math

counts = {}
snrs = {}

maxxy=180
channels=4

def count_grid(grid):
    counts[grid] = counts.get(grid, 0) + 1

def grid_coord(grid):
    return (
        (ord(grid[0])-ord('A')) * 10
        + int(grid[2]),
        (ord(grid[1])-ord('A')) * 10
        + int(grid[3])
    )
    
def generate_maidenheads():
    for long1 in range(ord('A'), ord('R')+1):
        for lat1 in range(ord('A'), ord('R')+1):
            for long2 in range(0,10):
                for lat2 in range(0,10):
                    grid = '%s%s%d%d' % (chr(long1),chr(lat1),long2,lat2)
                    yield grid, grid_coord(grid)

def color(val):
    if val < 0:
        val = 0
    elif val >= 1:
        val = 0.99999
    colors = [(0,0,255), (0,255,255), (0,255,0), (255,255,0), (255,0,0)]
    val = val * (len(colors)-1)
    idx1 = math.floor(val)
    idx2 = idx1+1
    frac = val - idx1
    return [
        int((colors[idx2][n] - colors[idx1][n])*frac + colors[idx1][n])
        for n in range(3) # RGB
    ]

def reg_snr(grid, snr):
    global snrs
    snrs.setdefault(grid, []).append(snr)
    
def main():
    call = '[a-zA-Z0-9]{1,3}[0123456789][a-zA-Z0-9]{0,3}[a-zA-Z]'
    grid = '[A-R][A-R]\d{2}'
    for line in sys.stdin:
        m = re.match(f'^([\d-]+)[, ]CQ {call} ({grid})', line)
        if m:
            count_grid(m[2])
            reg_snr(m[2], float(m[1]))

        m = re.match(f'^([\d-]+)[, ]{call} {call} ({grid})', line)
        if m:
            count_grid(m[2])
            reg_snr(m[2], float(m[1]))
    w,h = 3600,1800
    blockw,blockh = max(w//maxxy, 1), max(h//maxxy, 1)
    img = [[0 for _ in range(w*channels)] for _ in range(h)]

    # Make avgs.
    avgs = {}
    for maiden, snr in snrs.items():
        avgs[maiden] = sum(snr)/counts[maiden]

    # Normalize.
    mi,ma = min(avgs.values()),max(avgs.values())
    for maiden, val in avgs.items():
        avgs[maiden] = (val-mi)/(ma-mi)
    
    for maiden, (lo,la) in generate_maidenheads():
        if counts.get(maiden, 0) > 1:
            sx = (lo*w)//maxxy
            sy = (la*h)//maxxy
            c = color(avgs[maiden])
            for x in range(blockw+1):
                for y in range(blockh+1):
                    a = h - (sy + y)
                    b = (sx + x)*channels
                    img[a][b:b+3] = c
                    img[a][b+3] = 255 # Alpha channel.
        else:
            pass
            #print("Zero for %s" % maiden)
    with open('/dev/stdout', 'wb') as f:
        w=png.Writer(w,h,greyscale=False, alpha=True)
        w.write(f, img)

if __name__=='__main__':
    main()
    
