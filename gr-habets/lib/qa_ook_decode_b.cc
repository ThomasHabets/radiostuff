/* -*- c++ -*- */
/*
 * Copyright 2017 Thomas Habets <thomas@habets.se>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <gnuradio/attributes.h>
#include <cppunit/TestAssert.h>
#include "qa_ook_decode_b.h"
#include <habets/ook_decode_b.h>

namespace gr {
  namespace habets {
    uint8_t bits_to_byte(const char* b, const char* e);
    std::vector<uint8_t> bits_to_bytes(const std::vector<char>&);

    void
    qa_ook_decode_b::bits_to_byte()
    {
      std::vector<char> bits;
      CPPUNIT_ASSERT_EQUAL(uint8_t(0), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(1);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(0);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(1);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128+32), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(0);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128+32), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(1);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128+32+8), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(0);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128+32+8), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(1);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128+32+8+2), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));

      bits.push_back(0);
      CPPUNIT_ASSERT_EQUAL(uint8_t(128+32+8+2), gr::habets::bits_to_byte(&bits[0], &bits[bits.size()]));
    }

    void
    qa_ook_decode_b::bits_to_bytes()
    {
      std::vector<char> bits(9);
      bits[0] = 1;
      bits[1] = 1;
      bits[8] = 1;
      const std::vector<uint8_t> res = gr::habets::bits_to_bytes(bits);
      CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), res.size());
      CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(192), res[0]);
      CPPUNIT_ASSERT_EQUAL(static_cast<uint8_t>(128), res[1]);
    }

  } /* namespace ask */
} /* namespace gr */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
