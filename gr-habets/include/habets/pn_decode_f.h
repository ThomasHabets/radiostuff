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


#ifndef INCLUDED_HABETS_PN_DECODE_F_H
#define INCLUDED_HABETS_PN_DECODE_F_H

#include <habets/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief Positive/negative decoder.
     * \ingroup habets
     *
     */
    class HABETS_API pn_decode_f : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<pn_decode_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of habets::pn_decode_f.
       *
       * \param sps Estimated samples per symbol. \n
       * \param offset Offset into symbol to take sample. \n
       * \param epsilon Absolute value less than this is null. \n
       * \param nulls This many nulls in a row means end of packet.
       */
      static sptr make(int sps, int offset, float epsilon, int nulls);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_PN_DECODE_F_H */

