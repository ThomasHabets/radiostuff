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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/blocks/pdu.h>
#include <gnuradio/io_signature.h>
#include "ook_decode_b_impl.h"


namespace gr {
  namespace habets {
    // Trim off any trailing zeroes.
    std::vector<char> rtrim(const std::vector<char>& in)
    {
      std::vector<char> ret(in);
      size_t len = ret.size();
      while (len && !*ret.rbegin()) {
        ret.resize(--len);
      }
      return ret;
    }

    // Turn up to bits into a byte. MSB.
    // If fewer than 8 bits, the bits will be treated as the higher bits.
    uint8_t
    bits_to_byte(const char* b, const char* e)
    {
      uint8_t t = 0;
      const char* p = b;
      for (int c = 0; p < e; c++) {
        t <<= 1;
        if (*p++) {
          t |= 1;
        }
      }
      if (e - b < 8) {
        t <<= (8- (e-b));
      }
      return t;
    }

    // Turn a variable number of bits into bytes.
    std::vector<uint8_t>
    bits_to_bytes(const std::vector<char>& in)
    {
      const size_t len = in.size();
      std::vector<uint8_t> ret;
      for (size_t c = 0; c < len; c += 8) {
        ret.push_back(bits_to_byte(&in[c], &in[std::min(len, c+8)]));
      }
      return ret;
    }

    ook_decode_b::sptr
    ook_decode_b::make(int omega, int offset, int zeroes)
    {
      return gnuradio::get_initial_sptr
        (new ook_decode_b_impl(omega, offset, zeroes));
    }

    /*
     */
    ook_decode_b_impl::ook_decode_b_impl(int omega, int offset, int zeroes)
      : gr::sync_block("ook_decode_b",
              gr::io_signature::make(1, 1, sizeof(char)),
                       gr::io_signature::make(0, 0, 0)),
        omega_(omega), offset_(offset), zeroes_(zeroes)
    {
      message_port_register_out(PDU_PORT_ID);
      reset();
    }

    void
    ook_decode_b_impl::reset()
    {
      current_bits_.clear();
      current_zeroes_ = 0;
      next_expected_ = -1;
      last_ = false;
    }

    /*
     * Our virtual destructor.
     */
    ook_decode_b_impl::~ook_decode_b_impl()
    {
    }

    int
    ook_decode_b_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char *in = (const char*) input_items[0];
      int n;
      for (n = 0; n < noutput_items; n++) {
        const bool sample = static_cast<bool>(in[n]);
        if (sample && !last_) {
          // Up edge. Recalibrate.
          next_expected_ = offset_ + 1;
        }
        if (next_expected_ > 0) {
          next_expected_--;
        }
        if (!next_expected_) {
          // Record bit.
          current_bits_.push_back(sample);
          next_expected_ = omega_;
        }

        // Detect end of packet.
        current_zeroes_++;
        if (sample) {
          current_zeroes_ = 0;
        }
        if (current_zeroes_ == zeroes_ && !current_bits_.empty()) {
          // Send packet.
          const std::vector<char> p = rtrim(current_bits_);
          if (!p.empty()) {
            const std::vector<uint8_t> b = bits_to_bytes(p);
            const pmt::pmt_t vecpmt(pmt::make_blob(&b[0], b.size()));
            const pmt::pmt_t pdu(pmt::cons(pmt::PMT_NIL, vecpmt));
            message_port_pub(PDU_PORT_ID, pdu);
          }
          reset();
        }
        last_ = sample;
      }
      consume(0, n);
      return 0;
    }

  } /* namespace habets */
} /* namespace gr */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
