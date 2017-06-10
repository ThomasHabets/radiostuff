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


#ifndef INCLUDED_HABETS_PACKET_SIZE_FILTER_H
#define INCLUDED_HABETS_PACKET_SIZE_FILTER_H

#include <habets/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief Filter PDUs based on size.
     * \ingroup habets
     *
     */
    class HABETS_API packet_size_filter : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<packet_size_filter> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of habets::packet_size_filter.
       *
       * To avoid accidental use of raw pointers, habets::packet_size_filter's
       * constructor is in a private implementation
       * class. habets::packet_size_filter::make is the public interface for
       * creating new instances.
       *
       * \param min_size Reject all PDUs shorter than this. \n
       * \param max_size Reject all PDUs longer than this.
       */
      static sptr make(int min_size, int max_size);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_PACKET_SIZE_FILTER_H */

