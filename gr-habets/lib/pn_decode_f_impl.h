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

#ifndef INCLUDED_HABETS_PN_DECODE_F_IMPL_H
#define INCLUDED_HABETS_PN_DECODE_F_IMPL_H

#include <habets/pn_decode_f.h>

namespace gr {
  namespace habets {

    class pn_decode_f_impl : public pn_decode_f
    {
     private:
      // Samples per symbol estimate.
      const int sps_;

      // Offset into symbol to represent sample.
      const int offset_;

      // Less than abs(epsilon) means null.
      const float epsilon_;

      // How many nulls mean end of packet.
      const int nulls_;

      // State: nulls seen.
      int current_nulls_;

      // State: number of samples to skip before representing bit.
      int next_expected_;

      // Current bits in packet.
      std::vector<uint8_t> current_bits_;

      // State: Last candidate bit seen.
      // Used to detect up/down edges other than initial.
      uint8_t last_bit_;
      
      void reset();
      
     public:
      pn_decode_f_impl(int sps, int offset, float epsilon, int nulls);
      ~pn_decode_f_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_PN_DECODE_F_IMPL_H */

