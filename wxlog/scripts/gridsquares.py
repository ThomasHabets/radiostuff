#!/usr/bin/python3
# jq -r '[.Message] | @csv' wxlog.json  | sed 's/"//g' | python3 gridsquares.py
# composite -blend 50 foo.png Maidenhead_Locator_Map.png blah.png
import re
import sys
import png

counts = {}

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

def main():
    call = '[\w]+'
    grid = '[A-R][A-R]\d{2}'
    for line in sys.stdin:
        m = re.findall(f'^CQ {call} ({grid})', line)
        if m:
            count_grid(m[0])

        m = re.findall(f'^{call} {call} ({grid})', line)
        if m:
            count_grid(m[0])
    w,h = 3600,1800
    blockw,blockh = max(w//maxxy, 1), max(h//maxxy, 1)
    img = [[0 for _ in range(w*channels)] for _ in range(h)]
    for maiden, (lo,la) in generate_maidenheads():
        if counts.get(maiden, 0) > 1:
            sx = (lo*w)//maxxy
            sy = (la*h)//maxxy
            print(maiden, sx,sy)
            for x in range(blockw+1):
                for y in range(blockh+1):
                    a = h - (sy + y)
                    b = (sx + x)*channels
                    img[a][b] = 255
                    img[a][b+3] = 255
        else:
            pass
            #print("Zero for %s" % maiden)
    with open('foo.png', 'wb') as f:
        w=png.Writer(w,h,greyscale=False, alpha=True)
        w.write(f, img)

if __name__=='__main__':
    main()
    
