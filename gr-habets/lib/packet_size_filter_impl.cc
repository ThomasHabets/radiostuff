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
#include "packet_size_filter_impl.h"

namespace gr {
  namespace habets {

    packet_size_filter::sptr
    packet_size_filter::make(int min_size, int max_size)
    {
      return gnuradio::get_initial_sptr
        (new packet_size_filter_impl(min_size, max_size));
    }

    /*
     * The private constructor
     */
    packet_size_filter_impl::packet_size_filter_impl(int min_size, int max_size)
      : gr::block("packet_size_filter",
		  gr::io_signature::make(0, 0, 0),
                  gr::io_signature::make(0, 0, 0)),
      min_size_(min_size),max_size_(max_size)
    {
      message_port_register_in(PDU_PORT_ID);
      set_msg_handler(PDU_PORT_ID, [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t vector = pmt::cdr(msg);
          const size_t len = pmt::blob_length(vector);
          if (len < min_size_) {
            return;
          }
          if (len > max_size_) {
            return;
          }
          message_port_pub(PDU_PORT_ID, msg);
      });
      message_port_register_out(PDU_PORT_ID);
    }

    /*
     * Our virtual destructor.
     */
    packet_size_filter_impl::~packet_size_filter_impl()
    {
    }

    void
    packet_size_filter_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    packet_size_filter_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      // Will never be called.
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
