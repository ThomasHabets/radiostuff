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

import numpy
import time
from gnuradio import gr
from gnuradio.gr import pmt

def bits_to_byte(bits):
    t = 0
    for n in range(8):
        if bits[7-n]:
            t += 1 << n
    return t
                            

def bits_to_bytes(bits):
    for n in range(0, len(bits), 8):
        b = bits[n:n+8]
        if len(b) < 8:
            return
        yield bits_to_byte(b)

class pn_decode_bb(gr.basic_block):
    """
    docstring for block pn_decode_bb
    """
    def __init__(self, prefix):
        gr.basic_block.__init__(self,
            name="pn_decode_bb",
            in_sig=[numpy.byte],
            out_sig=[])
        self.message_port_register_out(pmt.intern("pdus"))
        self.packet = []
        self.prefix = prefix

    def forecast(self, noutput_items, ninput_items_required):
        #setup size of input_items[i] for work call
        for i in range(len(ninput_items_required)):
            ninput_items_required[i] = noutput_items

    def emit(self):
        if not len(self.packet):
            return
        payload = None
        for i in range(len(self.packet)):
            if self.packet[i:i+len(self.prefix)] == self.prefix:
                payload = self.packet[i:]
                break
        if payload:
            b = list(bits_to_bytes(payload))
            data = pmt.make_u8vector(len(b), ord(' '))
            for i in range(len(b)):
                pmt.u8vector_set(data, i, b[i])
            self.message_port_pub(pmt.intern('pdus'), pmt.cons(pmt.PMT_NIL, data))
        else:
            pass
            #print self.packet
        self.packet = []
        
    def general_work(self, input_items, output_items):
        for b in input_items[0]:
            if b == 0:
                self.emit()
            elif b < 0:
                self.packet.append(0)
            else:
                self.packet.append(1)
            self.consume(0, 1)
        return 0
