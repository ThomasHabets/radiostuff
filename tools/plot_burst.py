#!/usr/bin/python

import matplotlib.pyplot as plt
import scipy
import sys

def main():
    fn = sys.argv[1]
    f = scipy.fromfile(open(fn), dtype=scipy.complex64)
    plt.plot(f)
    plt.show()

if __name__ == '__main__':
    main()
