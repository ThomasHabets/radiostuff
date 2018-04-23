#!/usr/bin/python
#
# Tool to help tune a magnetic loop.
# Usage:
#
#  Just run it, and turn the knob a bit back and forth over the right area.
#
#  A new line means a new maximum was found.
#
#  Try to get meter as high as possible after it's stopped finding new
#  maximums.
#

import alextune
import time
import shutil
import os
import sys


def get_terminal_width():
    _, columns = os.popen('stty size', 'r').read().split()
    return int(columns)


def p(s):
    sys.stdout.write(s)
    sys.stdout.flush()
   

def main():
    tb = alextune.alextune()
    tb.start()
    mx = 0.0
    screenmax = 0.75
    try:
        print('Press ^C to quit\n')
        while True:
            time.sleep(0.1)
            v = tb.strength
            if v > mx:
                p('\n')
            mx = max(mx, v)
            w = get_terminal_width()
            l = int(screenmax * w * v / mx)
            tl = int(screenmax * w)
            p('\r' + ' ' * (int(w * screenmax) + 2))
            p('\r[' + '#' * l + ' ' * (tl - l) + ']')
        
    except KeyboardInterrupt:
        pass
    tb.stop()
    tb.wait()
    print('\n')


if __name__ == '__main__':
    main()
