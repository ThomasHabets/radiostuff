#!/usr/bin/python3

import sys
import math

def dist(a,b):
    "Return distance between two points of a sphere, in meters."
    R = 6371e3
    lat1,lon1 = [math.radians(x) for x in a]
    lat2,lon2 = [math.radians(x) for x in b]
    dlat = lat2 - lat1
    dlon = lon2 - lon1

    a = math.sin(dlat / 2)**2 + math.cos(lat1)*math.cos(lat2)*math.sin(dlon/2)**2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))
    return R*c

def decode_maidenhead(maiden):
    #print(maiden)
    mida = (ord('m')-97) / 24 + 1/48 - 90
    midb = (ord('m')-97) / 12 + 1/24 - 180
    if False:
        print(ord(maiden[1])-65)
        print(int(maiden[3]))
        print(mida)
    return (
        10*(ord(maiden[1])-65) + int(maiden[3]) + mida,
        20*(ord(maiden[0])-65) + int(maiden[2])*2 +midb
    )

def print_line(ts, vals):
    if vals:
        #print("%s %f" % (ts,max(vals)))
        print("%s %f" % (ts,sum(vals)/len(vals)))

def main():
    cur_ts = None
    values = []
    home = decode_maidenhead("IO91")
    for line in sys.stdin:
        n, freq, ts, maiden = line.lstrip(' ').rstrip('\n').split(' ')
        if sys.argv[1] != freq:
            continue
        try:
            coord = decode_maidenhead(maiden)
        except:
            continue
        if cur_ts != ts:
            print_line(cur_ts, values)
            values=[dist(home, coord)/1000]
            cur_ts = ts
        else:
            values.append(dist(home, coord)/1000)
    print_line(cur_ts, values)

if __name__ == '__main__':
    main()
