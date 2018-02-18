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

#ifndef INCLUDED_HABETS_FILE_PDU_GENERATOR_IMPL_H
#define INCLUDED_HABETS_FILE_PDU_GENERATOR_IMPL_H

#include <string>
#include <habets/file_pdu_generator.h>
#include <iostream>
#include <fstream>

namespace gr {
  namespace habets {

    class file_pdu_generator_impl : public file_pdu_generator
    {
     private:
      std::string contents_;

     public:
      file_pdu_generator_impl(const char* filename);
      ~file_pdu_generator_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_FILE_PDU_GENERATOR_IMPL_H */
