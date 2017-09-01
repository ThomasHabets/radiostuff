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
          // pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);
          const uint8_t* bits = static_cast<const uint8_t*>(pmt::blob_data(data));
          for (int c = 0; c < len; c++) {
            queue_.push_back(bits[c] ? 1.0 : -1);
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
    pn_source_f_impl::work(const int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      float* out = static_cast<float*>(output_items[0]);

      // Common case: no data.
      if (queue_.empty()) {
        memset(out, 0, sizeof(float) * noutput_items);
        return noutput_items;
      }

      // There is packet, inject as much of it as possible.
      const int packet_size = queue_.size();
      const int packet_part = std::min(packet_size, noutput_items);

      std::copy(queue_.begin(), queue_.begin() + packet_part, out);
      out += packet_part;

      // Delete sent bits from queue.
      if (packet_size == packet_part) {
        queue_.clear();
      } else {
        queue_.erase(queue_.begin(), queue_.begin() + (packet_size - packet_part));
      }

      // If more requested samples, fill up the rest with zeroes.
      if (noutput_items > packet_part) {
        memset(out, 0, sizeof(float) * (noutput_items - packet_size));
      }

      // We always produce as much as requested.
      return noutput_items;
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
