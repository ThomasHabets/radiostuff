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

import numpy

from gnuradio import gr
from gnuradio.gr import pmt

from wpcr import wpcr, slice_bits
            
                        
class magic_decoder(gr.basic_block):
    """
    docstring for block magic_decoder
    """
    def __init__(self):
        gr.basic_block.__init__(self,
            name="magic_decoder",
            in_sig=[],
            out_sig=[])
        self.port_out = pmt.intern("out")
        self.port_in = pmt.intern("in")
        self.port_info = pmt.intern("info")
        self.message_port_register_out(self.port_out)
        self.message_port_register_out(self.port_info)
        self.message_port_register_in(self.port_in)
        self.set_msg_handler(self.port_in, self._handle_msg)
       
    def _handle_msg(self, msg_pmt):
        meta = pmt.car(msg_pmt)
        bs = pmt.u8vector_elements(pmt.cdr(msg_pmt))
        fs = numpy.frombuffer(''.join([chr(x) for x in bs]), dtype=numpy.float32)
        #print "Got msg of size %d" % len(fs)
        #print midpoint(fs)
        misc, data = wpcr(fs)
        bits = slice_bits(data)
        vec = pmt.init_u8vector(len(bits), [int(x) for x in bits])
        meta = pmt.dict_add(meta, pmt.intern("magic_decoder"), pmt.to_pmt(misc))
        self.message_port_pub(self.port_out, pmt.cons(meta, vec))
        self.message_port_pub(self.port_info, meta)

    def forecast(self, noutput_items, ninput_items_required):
        #setup size of input_items[i] for work call
        for i in range(len(ninput_items_required)):
            ninput_items_required[i] = noutput_items

    def general_work(self, input_items, output_items):
        # No streams.
        return 0
