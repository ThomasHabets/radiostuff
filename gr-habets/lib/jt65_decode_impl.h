/* -*- c++ -*- */
/*
 * Copyright 2018 Thomas Habets <thomas@habets.se>
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

#ifndef INCLUDED_HABETS_JT65_DECODE_IMPL_H
#define INCLUDED_HABETS_JT65_DECODE_IMPL_H

#include <habets/jt65_decode.h>

namespace gr {
  namespace habets {

    class jt65_decode_impl : public jt65_decode
    {
     private:
      std::string decode(const std::vector<float>&) const;

      // Args:
      const int fft_size_ = 8192;
      const int samp_rate_ = 44100;
      const int buckets_per_symbol_ = 10;
      const int sps_ = 44100*0.372;

      // Calculated:
      const int batch_ = sps_/10;

     public:
      jt65_decode_impl();
      ~jt65_decode_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_JT65_DECODE_IMPL_H */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=2 sw=2
 */
