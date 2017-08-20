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
#include "file_pdu_generator_impl.h"

namespace gr {
  namespace habets {

    file_pdu_generator::sptr
    file_pdu_generator::make(const char* filename)
    {
      return gnuradio::get_initial_sptr
        (new file_pdu_generator_impl(filename));
    }

    /*
     * The private constructor
     */
    file_pdu_generator_impl::file_pdu_generator_impl(const char* filename)
      : gr::block("file_pdu_generator",
		  gr::io_signature::make(0,0,0),
		  gr::io_signature::make(0,0,0))
    {
      std::ifstream fi{filename, std::ios::binary | std::ios::ate};
      const size_t size = fi.tellg();
      contents_ = std::string(size, '\0');
      fi.seekg(0);
      if (!fi.read(&contents_[0], size)) {
        // TODO: better error handling.
        std::cerr << "oh noes!\n";
      }
      message_port_register_in(pmt::intern("trigger"));
      message_port_register_out(PDU_PORT_ID);
      set_msg_handler(pmt::intern("trigger"), [this](pmt::pmt_t msg) {
          const pmt::pmt_t vecpmt(pmt::make_blob(&contents_[0], contents_.size()));
          const pmt::pmt_t pdu(pmt::cons(pmt::make_dict(), vecpmt));
          message_port_pub(PDU_PORT_ID, pdu);
      });
    }

    /*
     * Our virtual destructor.
     */
    file_pdu_generator_impl::~file_pdu_generator_impl()
    {
    }

    void
    file_pdu_generator_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    file_pdu_generator_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
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
