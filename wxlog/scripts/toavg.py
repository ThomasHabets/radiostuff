#!/usr/bin/python3
#
# Take a sorted file of space-separated:
#  count frequency timestamp
#
# And for the lines with frequency=argv[1] produce:
#  average(count) timestamp
#
import sys

def print_line(ts, vals):
    if vals:
        print("%s %f" % (ts,sum(vals)/len(vals)))

def main():
    cur_ts = None
    values = []
    for line in sys.stdin:
        n, freq, ts = line.lstrip(' ').rstrip('\n').split(' ')
        if sys.argv[1] != freq:
            continue
        if cur_ts != ts:
            print_line(cur_ts, values)
            values=[int(n)]
            cur_ts = ts
        else:
            values.append(int(n))
    print_line(cur_ts, values)

if __name__ == '__main__':
    main()
