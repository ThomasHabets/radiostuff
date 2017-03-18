#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2017 <+YOU OR YOUR COMPANY+>.
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

from gnuradio import gr, gr_unittest
from gnuradio.gr import pmt
import socket
from gnuradio import blocks
import time
import habets_swig as habets

class qa_ook_decode_bb (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        for src_data, expected_result in (
            ( # A couple of bits.
                  [1,1,1, 0,0,0, 1,1,1, 0,0,0, 1,1,1, 0,0,0, 0,0,0, 0,0,0]
                + [0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0]
                + [1,1,1, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 1,1,1]
                + [0] * 1000,
                [168, 0, 129],
            ),
        ):
            src = blocks.vector_source_b(src_data)
            decode = habets.ook_decode_bb(3, 1, 100)
            dbg = blocks.message_debug()
            self.tb.connect(src, decode)
            self.tb.msg_connect(decode, "pdus", dbg, "store")

            self.tb.start()
            while dbg.num_messages() < 1:
                time.sleep(0.1)
            self.tb.stop()
            self.tb.wait()

            result_data = pmt.to_python(pmt.cdr(dbg.get_message(0)))

            try:
                self.assertFloatTuplesAlmostEqual(expected_result, result_data, 1)
            except AssertionError:
                print "--"
                print "Source: ", src_data
                print "Want", expected_result
                print "Got", result_data
                raise

if __name__ == '__main__':
    gr_unittest.run(qa_ook_decode_bb, "qa_ook_decode_bb.xml")
