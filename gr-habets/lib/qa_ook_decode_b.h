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

#ifndef _QA_OOK_DECODE_B_H_
#define _QA_OOK_DECODE_B_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestCase.h>

namespace gr {
  namespace habets {

    class qa_ook_decode_b : public CppUnit::TestCase {
    public:
      CPPUNIT_TEST_SUITE(qa_ook_decode_b);
      CPPUNIT_TEST(bits_to_byte);
      CPPUNIT_TEST(bits_to_bytes);
      CPPUNIT_TEST_SUITE_END();

    private:
      void bits_to_byte();
      void bits_to_bytes();
    };

  } /* namespace ask */
} /* namespace gr */

#endif /* _QA_OOK_DECODE_B_H_ */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
