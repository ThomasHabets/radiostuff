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


#ifndef INCLUDED_HABETS_OOK_DECODE_B_H
#define INCLUDED_HABETS_OOK_DECODE_B_H

#include <habets/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief Decode OOK
     * \ingroup habets
     *
     * More description here. Lalala.
     */
    class HABETS_API ook_decode_b : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<ook_decode_b> sptr;

      /*!
       * \brief Decode stream of bits into PDUs.
       *
       * Incoming bytes that are zero become zeroes, and everything
       * else becomes ones.
       *
       * \param omega Estimated width of a bit, in samples. \n
       * \param offset Index of sample into bit that is used. \n
       * \param zeroes How many samples of zeroes that mean "end of packet".
       */
      static sptr make(int omega, int offset, int zeroes);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_OOK_DECODE_B_H */

