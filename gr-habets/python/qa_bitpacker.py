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

from gnuradio import gr, gr_unittest
from gnuradio.gr import pmt
from gnuradio import blocks
import habets_swig as habets


class qa_bitpacker (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        for src_data, expected_result in (
                (
                    [],
                    [],
                ),
                (
                    [0],
                    [0],
                ),
                (
                    [1],
                    [128],
                ),
                (
                    [0,0,0,0,0,0,1],
                    [2],
                ),
                (
                    [1,0,0,0,0,0,0,0],
                    [128],
                ),
                (
                    [1,0,0,0,0,0,0,0,0,1],
                    [128, 64],
                ),
        ):
            pack = habets.bitpacker()
            dbg = blocks.message_debug()
            self.tb.msg_connect(pack, "pdus", dbg, "store")
            self.tb.start()
            pack.to_basic_block()._post(
                pmt.intern("pdus"),
                pmt.cons(
                    pmt.PMT_NIL,
                    pmt.init_u8vector(len(src_data), src_data)
                )
            )
            while dbg.num_messages() < 1:
                time.sleep(0.1)
            self.tb.stop()
            self.tb.wait()
            res = pmt.to_python(pmt.cdr(dbg.get_message(0)))
            try:
                self.assertFloatTuplesAlmostEqual(expected_result, res, 1)
            except AssertionError:
                print "--"
                print "Source:", src_data
                print "Want:  ", expected_result
                print "Got:   ", res
                raise


if __name__ == '__main__':
    gr_unittest.run(qa_bitpacker, "qa_bitpacker.xml")
