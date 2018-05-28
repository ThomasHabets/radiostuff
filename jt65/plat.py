#!/usr/bin/python
#
# Given a list of samples create a new list of samples that's plateued.
#

import numpy

width = 32  # Samples per symbol, approximately.
wiggle = 8  # How many samples to wiggle around.


def pick_value(a):
    """From a list of samples, pick plateu value."""
    return numpy.median(numpy.array(a))

def descend(pos, wiggle, stds):
    """Find a best place to choose as sample boundary."""
    partial = stds[pos-wiggle:pos+wiggle]
    smallest = min(partial, key=lambda x: x[0])
    return smallest[2]

def main():
    samples = [float(x) for x in open('t')]
    running_sum = sum(samples[:width])

    stds = []
    for n, s in enumerate(samples[width:]):
        window = numpy.array(samples[n:n+width])
        stds.append( (numpy.std(window), numpy.mean(window), n) )

    open('stddev', 'w').write('\n'.join([str(x[0]) for x in stds]))
    y,m,x = min(stds)
    open('min', 'w').write("%f %d\n" % (x,y))

    symbols = [0] * len(samples)
    pos = x
    symbols[pos:pos+width] = (m for _ in range(width))

    # Fill in backwards.
    while pos > 2*width:
        newpos = pos - width

        newpos = descend(newpos, wiggle, stds)
        m = pick_value(samples[newpos:newpos+width])
        #m = stds[newpos][1]
    
        #print newpos-width,newpos,len(symbols),m
        symbols[newpos:newpos+width] = (m for _ in range(width))
        pos = newpos

    # Fill in forwards.
    pos = x
    while pos < len(samples)-2*width:
        newpos = pos + width

        newpos = descend(newpos, wiggle, stds)
        m = pick_value(samples[newpos:newpos+width])
    
        symbols[newpos:newpos+width] = (m for _ in range(width))
        pos = newpos

    open('symbols', 'w').write('\n'.join([str(x) for x in symbols]) + '\n')

if __name__ == '__main__':
    main()
