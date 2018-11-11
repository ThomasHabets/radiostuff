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
#include "tagged_stream_to_large_pdu_f_impl.h"
namespace {
  const bool debug = true;
}

namespace gr {
  namespace habets {

    tagged_stream_to_large_pdu_f::sptr
    tagged_stream_to_large_pdu_f::make(const std::string& length_tag)
    {
      return gnuradio::get_initial_sptr
        (new tagged_stream_to_large_pdu_f_impl(length_tag));
    }

    /*
     * The private constructor
     */
    tagged_stream_to_large_pdu_f_impl::tagged_stream_to_large_pdu_f_impl(const std::string& length_tag)
      : gr::sync_block("tagged_stream_to_large_pdu_f",
                       gr::io_signature::make(1, 1, sizeof(float)),
                       gr::io_signature::make(0, 0, 0)),
        length_tag_(length_tag)
    {
      message_port_register_out(pdu_port_id_);
    }

    /*
     * Our virtual destructor.
     */
    tagged_stream_to_large_pdu_f_impl::~tagged_stream_to_large_pdu_f_impl()
    {
    }

    int
    tagged_stream_to_large_pdu_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      // If we're not in a tag, look for one.
      if (current_length_ == 0) {
        std::vector<tag_t> tags;
        get_tags_in_window(tags, 0, 0, noutput_items);
        const auto relative_offset = nitems_read(0);

        // API apparently doesn't guarantee tag order.
        std::sort(tags.begin(), tags.end(), tag_t::offset_compare);
        for (const auto& t : tags) {
          const auto name = pmt::symbol_to_string(t.key);
          const auto value = pmt::to_long(t.value);
          if (debug) {
            std::clog << "Tag: " << pmt::symbol_to_string(t.key) << "@" << value << std::endl;
          }
          if (pmt::symbol_to_string(t.key) == length_tag_) {
            current_length_ = value;
            const auto ofs = t.offset - relative_offset;
            std::clog << "Packet length at offset " << ofs << std::endl;
            if (ofs != 0) {
              // Discard up to tag.
              return ofs;
            }
          }
        }
      }

      const float* in = (const float*)input_items[0];
      const auto to_read = std::min(static_cast<size_t>(noutput_items), current_length_ - buffer_.size());
      std::copy(&in[0], &in[to_read], std::back_inserter(buffer_));
      if (buffer_.size() == current_length_) {
        const pmt::pmt_t vecpmt(pmt::make_blob(&buffer_[0], sizeof(float)*buffer_.size()));
        const pmt::pmt_t pdu(pmt::cons(pmt::PMT_NIL, vecpmt));
        message_port_pub(pdu_port_id_, pdu);
        buffer_.clear();
        current_length_ = 0;
      }

      // Tell runtime system how many output items we consumed.
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
 * vim: ts=2 sw=2
 */
