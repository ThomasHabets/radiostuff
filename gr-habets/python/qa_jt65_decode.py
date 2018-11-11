#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright 2018 Thomas Habets <thomas@habets.se>
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
import struct
import random

from gnuradio import gr, gr_unittest
from gnuradio.gr import pmt
from gnuradio import blocks
import habets_swig as habets

class qa_jt65_decode (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        try:
            fs = open("jt65.floats").read()
        except IOError:
            print "Test input not found; skipping unit test"
            return
        fs = list(struct.unpack("f"*(len(fs)/4), fs))
        rnd = lambda x: [random.randint(-120,120) for _ in range(x)]

        for name, src_data, expected_result in (
                ('clean', fs, "CQ M0THC IO91"),
                ('random-prefix', rnd(44100*2) + fs, "CQ M0THC IO91"),
        ):
            # print 'Making data'
            pmt_data = pmt.cons(
                pmt.PMT_NIL,
                pmt.init_f32vector(len(src_data), list(src_data))
            )

            # print 'Making flowgraph'
            dec = habets.jt65_decode(44100, int(44100*0.372), 10, 8192)
            dbg = blocks.message_debug()
            self.tb.msg_connect(dec,"out", dbg, "store")

            # print 'Starting flowgraph'
            self.tb.start()

            # print 'Posting message'
            dec.to_basic_block()._post(
                pmt.intern("in"),
                pmt_data,
            )

            # print 'Waiting for message'
            while dbg.num_messages() < 1:
                time.sleep(0.1)

            # print 'Stopping flowgraph'
            self.tb.stop()
            self.tb.wait()
            # print 'Getting reply'
            res = pmt.to_python(pmt.cdr(dbg.get_message(0)))
            res = ''.join([chr(x) for x in res])
            try:
                # print res
                assert res == expected_result
                print "%s OK" % name
            except AssertionError:
                print "--"
                print "Want:  ", expected_result
                print "Got:   ", res
                raise


if __name__ == '__main__':
    gr_unittest.run(qa_jt65_decode, "qa_jt65_decode.xml")
