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

########################################################################
# Include python install macros
########################################################################
include(GrPython)
if(NOT PYTHONINTERP_FOUND)
    return()
endif()

########################################################################
# Install python sources
########################################################################
GR_PYTHON_INSTALL(
    FILES
    __init__.py
    pn_decode_identity_b.py
    preamble_stripper.py
    preamble_adder.py
    wpcr.py
    magic_decoder.py
    prototype_tagged_stream_to_vector.py DESTINATION ${GR_PYTHON_DIR}/habets
)

########################################################################
# Handle the unit tests
########################################################################
include(GrTest)

set(GR_TEST_TARGET_DEPS gnuradio-habets)
set(GR_TEST_PYTHON_DIRS ${CMAKE_BINARY_DIR}/swig)
GR_ADD_TEST(qa_ook_decode_b ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_ook_decode_b.py)
GR_ADD_TEST(qa_pn_decode_identity_b ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_pn_decode_identity_b.py)
GR_ADD_TEST(qa_packet_size_filter ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_packet_size_filter.py)
GR_ADD_TEST(qa_pn_decode_f ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_pn_decode_f.py)
GR_ADD_TEST(qa_bitpacker ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_bitpacker.py)
GR_ADD_TEST(qa_pn_source_f ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_pn_source_f.py)
GR_ADD_TEST(qa_file_pdu_generator ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_file_pdu_generator.py)
GR_ADD_TEST(qa_bitunpacker ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_bitunpacker.py)
GR_ADD_TEST(qa_preamble_stripper ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_preamble_stripper.py)
GR_ADD_TEST(qa_preamble_adder ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_preamble_adder.py)
GR_ADD_TEST(qa_packetize_burst ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_packetize_burst.py)
GR_ADD_TEST(qa_magic_decoder ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_magic_decoder.py)
GR_ADD_TEST(qa_stairclocker ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_stairclocker.py)
GR_ADD_TEST(qa_float_streamer ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_float_streamer.py)
GR_ADD_TEST(qa_prototype_tagged_stream_to_vector ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_prototype_tagged_stream_to_vector.py)
GR_ADD_TEST(qa_jt65_encode ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_jt65_encode.py)
GR_ADD_TEST(qa_jt65_add_sync ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_jt65_add_sync.py)
GR_ADD_TEST(qa_jt65_decode ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_jt65_decode.py)
GR_ADD_TEST(qa_tagged_stream_to_large_pdu_f ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/qa_tagged_stream_to_large_pdu_f.py)
