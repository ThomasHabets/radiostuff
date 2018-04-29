#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2018 <+YOU OR YOUR COMPANY+>.
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

import numpy
import time
from gnuradio import gr
from gnuradio.gr import pmt

class prototype_tagged_stream_to_vector(gr.basic_block):
    """
    docstring for block prototype_tagged_stream_to_vector
    """
    def __init__(self, packet_len, packet_length_tag):
        gr.basic_block.__init__(self,
                                name="prototype_tagged_stream_to_vector",
                                in_sig=[numpy.byte],
                                out_sig=[(numpy.byte, packet_len)])
        self._tag = packet_length_tag
        self._packet_len = packet_len
        self._last = time.time()

    def forecast(self, noutput_items, ninput_items_required):
        #setup size of input_items[i] for work call
        for i in range(len(ninput_items_required)):
            ninput_items_required[i] = self._packet_len * 10

    def general_work(self, input_items, output_items):
        window_size = len(input_items[0])
        tags = self.get_tags_in_window(0, 0, window_size)
        ab = self.nitems_read(0)

        #print "block of %d, tags: %d" % (window_size, len(tags))
        old = 0
        for t in tags:
            new = int(t.offset)
            #print "  %d %s" % (t.offset - ab, t.key)
            if new < old:
                print "WHAAAA? Tags are not sorted?"
            old = new

        for t in tags:
            # Ignore wrong tags.
            if str(t.key) != self._tag:
                #print "Wrong tag %s" % str(t.key)
                continue

            # Ignore tags of wrong size.
            if pmt.to_python(t.value) != self._packet_len:
                #print "Wrong len %d" % pmt.to_python(t.value)
                continue
            
            rel_offset = t.offset - self.nitems_read(0)

            # For easy implementation, skip to first tag.
            if rel_offset != 0:
                #print "Got tag at offset %d. Nope" % (rel_offset)
                self.consume(0, rel_offset)
                return 0

            now = time.time()
            
            # Tag is at zero. Go.
            data = input_items[0][:self._packet_len]
            #print "%.6f packet from abs offset %d, taglen %d" % ((now - self._last)*1000.0, self.nitems_read(0), len(tags))
            self._last  = now
            output_items[0][0] = data
            self.consume(0, 1)
            return 1

        # No tags.
        self.consume(0, len(input_items[0]))
        return 0
