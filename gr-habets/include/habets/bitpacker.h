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


#ifndef INCLUDED_HABETS_BITPACKER_H
#define INCLUDED_HABETS_BITPACKER_H

#include <habets/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief Pack PDU of bits into PDU of bytes.
     * \ingroup habets
     *
     */
    class HABETS_API bitpacker : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<bitpacker> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of habets::bitpacker.
       *
       * To avoid accidental use of raw pointers, habets::bitpacker's
       * constructor is in a private implementation
       * class. habets::bitpacker::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_BITPACKER_H */

