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
#include "bitunpacker_impl.h"

namespace gr {
  namespace habets {

    std::vector<uint8_t>
    bytes_to_bits(const std::vector<uint8_t>& in)
    {
      std::vector<uint8_t> ret;
      for (auto& b : in) {
        for (int c = 7; c >= 0; --c) {
          ret.push_back((b >> c) & 1);
        }
      }
      return ret;
    }
    
    bitunpacker::sptr
    bitunpacker::make()
    {
      return gnuradio::get_initial_sptr
        (new bitunpacker_impl());
    }

    /*
     * The private constructor
     */
    bitunpacker_impl::bitunpacker_impl()
      : gr::block("bitunpacker",
		  gr::io_signature::make(0,0,0),
                  gr::io_signature::make(0,0,0))
    {
      std::cerr << "Setup\n";
      std::cout << "Setup\n";
      message_port_register_in(pmt::intern("in"));
      set_msg_handler(pmt::intern("in"), [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);
          const uint8_t* bits = static_cast<const uint8_t*>(pmt::blob_data(data));
          const std::vector<uint8_t> out = bytes_to_bits(std::vector<uint8_t>(&bits[0], &bits[len]));
          const pmt::pmt_t vecpmt(pmt::make_blob(&out[0], out.size()));
          const pmt::pmt_t pdu(pmt::cons(meta, vecpmt));
          std::cerr << "Publish a PDU\n";
          message_port_pub(pmt::intern("out"), pdu);
      });
      message_port_register_out(pmt::intern("out"));
    }

    /*
     * Our virtual destructor.
     */
    bitunpacker_impl::~bitunpacker_impl()
    {
    }

    void
    bitunpacker_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    bitunpacker_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      // Tell runtime system how many output items we produced.
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
