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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "float_streamer_impl.h"

namespace gr {
  namespace habets {

    float_streamer::sptr
    float_streamer::make(const int padding)
    {
      return gnuradio::get_initial_sptr(new float_streamer_impl(padding));
    }

    /*
     * The private constructor
     */
    float_streamer_impl::float_streamer_impl(int padding)
      : gr::sync_block("float_streamer",
                       gr::io_signature::make(0, 0, 0),
                       gr::io_signature::make(1, 1, sizeof(float))),
        padding_(padding),
        port_in_(pmt::intern("in"))
    {
      message_port_register_in(port_in_);
      set_msg_handler(port_in_, [this](pmt::pmt_t msg) {
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data) / sizeof(float);
          const float* in = static_cast<const float*>(pmt::blob_data(data));
          queue_.reserve(queue_.size() + padding_ + len);
          std::copy(&in[0], &in[len], std::back_inserter(queue_));
          queue_.resize(queue_.size() + padding_);
      });
    }

    /*
     * Our virtual destructor.
     */
    float_streamer_impl::~float_streamer_impl()
    {
    }

    int
    float_streamer_impl::work(int noutput_items,
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
