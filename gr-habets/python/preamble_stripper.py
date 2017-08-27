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

from gnuradio import gr
from gnuradio.gr import pmt

class preamble_stripper(gr.basic_block):
    """
    Strip preamble.

    TODO: put search length into parameters.
    """
    def __init__(self, prefix):
        gr.basic_block.__init__(self,
            name="preamble_stripper",
            in_sig=[],
            out_sig=[])
        self.message_port_register_out(pmt.intern('out'))
        self.message_port_register_in(pmt.intern('in'))
        self.set_msg_handler(pmt.intern('in'), self._handle_msg)
        self.prefix = prefix
        self.search = len(prefix)*2

    def _handle_msg(self, msg_pmt):
        bits = pmt.u8vector_elements(pmt.cdr(msg_pmt))
        
        found = None
        for i in range(self.search-1, -1, -1):
            candidate = list(bits[i:i+len(self.prefix)])
            if candidate == self.prefix:
                found = i
                break
        if found is None:
            return
        bits = bits[found+len(self.prefix):]
        vec = pmt.init_u8vector(len(bits), bits)
        self.message_port_pub(pmt.intern('out'), pmt.cons(pmt.car(msg_pmt), vec))

    def forecast(self, noutput_items, ninput_items_required):
        # Does not consume streams.
        pass

    def general_work(self, input_items, output_items):
        return 0
