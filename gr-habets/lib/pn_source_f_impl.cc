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
#include "pn_source_f_impl.h"

namespace gr {
  namespace habets {

    pn_source_f::sptr
    pn_source_f::make()
    {
      return gnuradio::get_initial_sptr
        (new pn_source_f_impl());
    }

    /*
     * The private constructor
     */
    pn_source_f_impl::pn_source_f_impl()
      : gr::sync_block("pn_source_f",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
      message_port_register_in(PDU_PORT_ID);
      set_msg_handler(PDU_PORT_ID, [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);
          const uint8_t* bits = static_cast<const uint8_t*>(pmt::blob_data(data));
          for (int c = 0; c < len; c++) {
            queue_.push(bits[c]);
          }
      });
    }

    /*
     * Our virtual destructor.
     */
    pn_source_f_impl::~pn_source_f_impl()
    {
    }

    int
    pn_source_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      float* out = (float*) output_items[0];

      for (int c = 0; c < output_items.size(); c++) {
        if (queue_.empty()) {
          *out++ = 0.0;
        } else {
          *out++ = queue_.front() ? 1 : -1;
          queue_.pop();
        }
      }

      // Tell runtime system how many output items we produced.
      return output_items.size();
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
