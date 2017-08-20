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


#ifndef INCLUDED_HABETS_FILE_PDU_GENERATOR_H
#define INCLUDED_HABETS_FILE_PDU_GENERATOR_H

#include <habets/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief Generates a PDU from file when triggered.
     * \ingroup habets
     *
     */
    class HABETS_API file_pdu_generator : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<file_pdu_generator> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of habets::file_pdu_generator.
       *
       * To avoid accidental use of raw pointers, habets::file_pdu_generator's
       * constructor is in a private implementation
       * class. habets::file_pdu_generator::make is the public interface for
       * creating new instances.
       */
      static sptr make(const char* filename);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_FILE_PDU_GENERATOR_H */

