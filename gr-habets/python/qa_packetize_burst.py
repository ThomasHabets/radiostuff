#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2017 Thomas Habets <thomas@habets.se>
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import time
import numpy

from gnuradio import gr, gr_unittest
from gnuradio.gr import pmt
from gnuradio import blocks
import habets_swig as habets


def make_tag(offset, key, value, srcid=None):
    tag = gr.tag_t()
    tag.key = pmt.string_to_symbol(key)
    tag.value = pmt.to_pmt(value)
    tag.offset = offset
    if srcid is not None:
        tag.srcid = pmt.to_pmt(srcid)
    return tag

class qa_packetize_burst (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        for name, src_data, tags, packets in (
                (
                    "One packet exactly",
                    [1, 0],
                    [
                        make_tag(0, 'burst', True),
                        make_tag(1, 'burst', False),
                    ],
                    [
                        [1, 0],
                    ],
                ),
                (
                    "One packet with spacing",
                    [0,1,0,0],
                    [
                        make_tag(1, 'burst', True),
                        make_tag(2, 'burst', False),
                    ],
                    [
                        [1, 0],
                    ],
                ),
                (
                    "Two packets without spacing",
                    [9, 1, 8, -1, -2],
                    [
                        make_tag(1, 'burst', True),
                        make_tag(2, 'burst', False),
                        make_tag(3, 'burst', True),
                        make_tag(4, 'burst', False),
                    ],
                    [
                        [1, 8],
                        [-1, -2],
                    ],
                ),
                (
                    "Two packets with spacing",
                    [9, 1, 8, 7, -1, -2],
                    [
                        make_tag(1, 'burst', True),
                        make_tag(2, 'burst', False),
                        make_tag(4, 'burst', True),
                        make_tag(5, 'burst', False),
                    ],
                    [
                        [1, 8],
                        [-1, -2],
                    ],
                ),
        ):
            src = blocks.vector_source_f(src_data, tags=tags)
            p = habets.packetize_burst('burst')
            dbg = blocks.message_debug()
            self.tb.connect(src, p)
            self.tb.msg_connect(p, "pdus", dbg, "store")
            self.tb.start()
            while dbg.num_messages() < len(packets):
                time.sleep(0.1)
            self.tb.stop()
            self.tb.wait()

            for n, packet in enumerate(packets):
                r = pmt.to_python(pmt.cdr(dbg.get_message(n)))
                result_data = numpy.frombuffer(''.join([chr(x) for x in r]), dtype=numpy.float32)

                try:
                    self.assertFloatTuplesAlmostEqual(packet, result_data, 1)
                except AssertionError:
                    print "--"
                    print "Test name: ", name
                    print "Output packet index: ", n
                    print "Source: ", src_data
                    print "Want", packets
                    print "Got", result_data
                    raise


if __name__ == '__main__':
    gr_unittest.run(qa_packetize_burst, "qa_packetize_burst.xml")
