# Copyright 2011,2012,2016 Free Software Foundation, Inc.
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
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND habets_sources
    ook_decode_b_impl.cc
    packet_size_filter_impl.cc
    pn_decode_f_impl.cc
    bitpacker_impl.cc
    pn_source_f_impl.cc
)

set(habets_sources "${habets_sources}" PARENT_SCOPE)
if(NOT habets_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT habets_sources)

add_library(gnuradio-habets SHARED ${habets_sources})
target_link_libraries(gnuradio-habets ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES})
set_target_properties(gnuradio-habets PROPERTIES DEFINE_SYMBOL "gnuradio_habets_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-habets PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
include(GrMiscUtils)
GR_LIBRARY_FOO(gnuradio-habets RUNTIME_COMPONENT "habets_runtime" DEVEL_COMPONENT "habets_devel")

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_habets_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_habets.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_habets.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ook_decode_b_impl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_ook_decode_b.cc
)
# TODO: Impl should not be in that list.

add_executable(test-habets ${test_habets_sources})

target_link_libraries(
  test-habets
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  gnuradio-habets
  gnuradio-pmt
)

GR_ADD_TEST(test_habets test-habets)

########################################################################
# Print summary
########################################################################
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Building for version: ${VERSION} / ${LIBVER}")
