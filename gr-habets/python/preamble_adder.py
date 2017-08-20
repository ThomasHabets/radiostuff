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

class preamble_adder(gr.basic_block):
    """
    docstring for block preamble_adder
    """
    def __init__(self, preamble):
        gr.basic_block.__init__(self,
            name="preamble_adder",
            in_sig=[],
            out_sig=[])
        self.message_port_register_out(pmt.intern('out'))
        self.message_port_register_in(pmt.intern('in'))
        self.set_msg_handler(pmt.intern('in'), self._handle_msg)
        self.preamble = preamble

    def _handle_msg(self, msg_pmt):
        bits = list(self.preamble) + list(pmt.u8vector_elements(pmt.cdr(msg_pmt)))
        vec = pmt.make_u8vector(len(bits), 0)
        for n, b in enumerate(bits):
            pmt.u8vector_set(vec, n, b)
        self.message_port_pub(pmt.intern('out'), pmt.cons(pmt.PMT_NIL, vec))
        
    def forecast(self, noutput_items, ninput_items_required):
        #setup size of input_items[i] for work call
        for i in range(len(ninput_items_required)):
            ninput_items_required[i] = noutput_items

    def general_work(self, input_items, output_items):
        return 0
