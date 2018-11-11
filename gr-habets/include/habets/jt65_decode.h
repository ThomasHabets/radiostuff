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


#ifndef INCLUDED_HABETS_JT65_DECODE_H
#define INCLUDED_HABETS_JT65_DECODE_H

#include <habets/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief JT65 decoder supporting all flavours of JT65.
     * \ingroup habets
     *
     * Input: A whole JT65 burst as floats (audio).
     * Output: Decoded packets as strings.
     */
    class HABETS_API jt65_decode : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<jt65_decode> sptr;

      /*!
       * \brief Create a JT65 decoder.
       *
       * \param samp_rate Sample rate of input.
       * \param sps Samples per symbol. Standard JT65 uses 0.372 seconds. JT65B2 and JT65C2 are half that.
       * \param buckets_per_symbol How many FFTs done per symbol. TODO: Explain tradeoff.
       * \param fft_size FFT size. FFT must be larger than sps/buckets_per_symbol, and will be padded with zeroes to size. Increasing FFT size improves frequency resolution at the cost of CPU. FFT is O(N*logN) with this number.
       * \param symbol_offset Frequency distance between two symbols in Hertz. 2.7Hz for JT65A, 5.4Hz for JT65B, 10.8Hz for JT65C.
       */
      static sptr make(int samp_rate, int sps, int buckets_per_symbol, int fft_size, float symbol_offset);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_JT65_DECODE_H */
