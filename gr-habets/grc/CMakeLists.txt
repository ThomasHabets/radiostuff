# Copyright 2011 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

install(FILES
    habets_ook_decode_b.xml
    habets_pn_decode_identity_b.xml
    habets_packet_size_filter.xml
    habets_pn_decode_f.xml
    habets_bitpacker.xml
    habets_pn_source_f.xml
    habets_file_pdu_generator.xml
    habets_bitunpacker.xml
    habets_preamble_stripper.xml
    habets_preamble_adder.xml
    habets_packetize_burst.xml
    habets_magic_decoder.xml
    habets_stairclocker.xml
    habets_float_streamer.xml
    habets_prototype_tagged_stream_to_vector.xml DESTINATION share/gnuradio/grc/blocks
)
