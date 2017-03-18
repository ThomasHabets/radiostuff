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

#ifndef INCLUDED_HABETS_OOK_DECODE_BB_IMPL_H
#define INCLUDED_HABETS_OOK_DECODE_BB_IMPL_H

#include <habets/ook_decode_bb.h>

namespace gr {
  namespace habets {

    class ook_decode_bb_impl : public ook_decode_bb {
    private:
      // Expected bit spacing in samples.
      const int omega_;

      // Expected samples from rising edge to middle of pulse.
      const int offset_;

      // Number of zero samples to trigger "end of packet".
      const int zeroes_;

      // Samples to discard before capturing a bit.
      // If <0 then not capturing bits.
      int next_expected_;

      // Bits captured so far in packet.
      std::vector<char> current_bits_;

      // Counter of zero samples. See `zeroes_`.
      int current_zeroes_;

      // Last sample value. Used to detect rising edge.
      bool last_;

      // Reset state to "not in packet".
      void reset();

    public:
      ook_decode_bb_impl(int omega, int offset, int zeroes);
      ~ook_decode_bb_impl();

      int work(int noutput_items,
               gr_vector_const_void_star &input_items,
               gr_vector_void_star &output_items);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_OOK_DECODE_BB_IMPL_H */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
