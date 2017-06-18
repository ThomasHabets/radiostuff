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
#include "bitpacker_impl.h"

namespace gr {
  namespace habets {

    // Turn up to bits into a byte. MSB.
    // If fewer than 8 bits, the bits will be treated as the higher bits.
    uint8_t
    bits_to_byte(const uint8_t* b, const uint8_t* e)
    {
      uint8_t t = 0;
      const uint8_t* p = b;
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
    bits_to_bytes(const std::vector<uint8_t>& in)
    {
      if (in.empty()) {
        return in;
      }
      const size_t len = in.size();
      std::vector<uint8_t> ret;
      for (size_t c = 0; c < len; c += 8) {
        ret.push_back(bits_to_byte(&in[c], &in[std::min(len, c+8)]));
      }
      return ret;
    }

    bitpacker::sptr
    bitpacker::make()
    {
      return gnuradio::get_initial_sptr(new bitpacker_impl());
    }

    /*
     * The private constructor
     */
    bitpacker_impl::bitpacker_impl()
      : gr::block("bitpacker",
		  gr::io_signature::make(0, 0, 0),
                  gr::io_signature::make(0, 0, 0))
    {
      message_port_register_in(PDU_PORT_ID);
      set_msg_handler(PDU_PORT_ID, [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);
          const uint8_t* bits = static_cast<const uint8_t*>(pmt::blob_data(data));
          const std::vector<uint8_t> out = bits_to_bytes(std::vector<uint8_t>(&bits[0], &bits[len]));
          const pmt::pmt_t vecpmt(pmt::make_blob(&out[0], out.size()));
          const pmt::pmt_t pdu(pmt::cons(meta, vecpmt));
          message_port_pub(PDU_PORT_ID, pdu);
      });
      message_port_register_out(PDU_PORT_ID);
    }

    /*
     * Our virtual destructor.
     */
    bitpacker_impl::~bitpacker_impl()
    {
    }

    void
    bitpacker_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    bitpacker_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      // Never called.
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
