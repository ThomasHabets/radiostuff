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
#include "pn_decode_f_impl.h"

namespace {
  const bool verbose = false;
}

namespace gr {
  namespace habets {

    pn_decode_f::sptr
    pn_decode_f::make(int sps, int offset, float epsilon, int nulls)
    {
      return gnuradio::get_initial_sptr
        (new pn_decode_f_impl(sps, offset, epsilon, nulls));
    }

    /*
     * The private constructor
     */
    pn_decode_f_impl::pn_decode_f_impl(int sps, int offset, float epsilon, int nulls)
      : gr::sync_block("pn_decode_f",
		       gr::io_signature::make(1, 1, sizeof(float)),
		       gr::io_signature::make(0, 0, 0)),
      sps_(sps), offset_(offset), epsilon_(epsilon), nulls_(nulls)
    {
      message_port_register_out(PDU_PORT_ID);
      reset();
    }

    /*
     * Our virtual destructor.
     */
    pn_decode_f_impl::~pn_decode_f_impl()
    {
    }

    void
    pn_decode_f_impl::reset()
    {
      current_bits_.clear();
      current_nulls_ = 0;
      next_expected_ = -1;
      last_bit_ = 2; // Neither 0 nor 1.
    }
    
    int
    pn_decode_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* in = static_cast<const float*>(input_items[0]);

      // Do signal processing.
      int n;
      if (verbose) {
        std::cerr << "Entering work()\n";
      }
      for (n = 0; n < noutput_items; n++) {
        const float sample = static_cast<float>(in[n]);
        if (verbose) {
          std::cerr << "Sample: " << sample
                    << " last_bit_=" << int(last_bit_)
                    << " next_expected=" << next_expected_
                    << std::endl;
        }
        const bool already_in_packet = next_expected_ >= 0;

        // Look for up/down edges.
        if (next_expected_ != 0 && fabs(sample) > epsilon_) {
          bool recalibrate = false;
          if (!already_in_packet) {
            recalibrate = true;
          }

          if (last_bit_ == 0 && sample > 0) {
            recalibrate = true;
          }
          if (last_bit_ == 1 && sample < 0) {
            recalibrate = true;
          }
          if (recalibrate) {
            if (verbose) {
              std::cerr << "  Recalibrating\n";
            }
            next_expected_ = offset_ + 1;
          }
        }

        if (next_expected_ > 0) {
          next_expected_--;
        }

        if (fabs(sample) > epsilon_) {
          last_bit_ = sample > 0;
        }

        
        if (next_expected_ == 0) {
          // Record bit (unless it's null).
          // If it's null then the next sample will be attempted.
          if (fabs(sample) > epsilon_) {
            const uint8_t bit = sample > 0;
            if (verbose) {
              std::cerr << "  Registering " << int(bit) << std::endl;
            }
            current_bits_.push_back(bit);
            next_expected_ = sps_;
          }
        }

        // Count nulls.
        current_nulls_++;
        if (fabs(sample) > epsilon_) {
          current_nulls_ = 0;
        }

        if (current_nulls_ == nulls_ && !current_bits_.empty()) {
          // Send packet.
          const pmt::pmt_t vecpmt(pmt::make_blob(&current_bits_[0], current_bits_.size()));
          const pmt::pmt_t pdu(pmt::cons(pmt::PMT_NIL, vecpmt));
          message_port_pub(PDU_PORT_ID, pdu);
          reset();
        }
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
