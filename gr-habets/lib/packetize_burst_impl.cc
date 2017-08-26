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

#include <gnuradio/block.h>
#include <gnuradio/blocks/pdu.h>
#include <gnuradio/io_signature.h>
#include "packetize_burst_impl.h"

static const bool debug = false;

namespace gr {
  namespace habets {

    packetize_burst::sptr
    packetize_burst::make(const char* tag, int max_noutput_items)
    {
      return gnuradio::get_initial_sptr(
          new packetize_burst_impl(tag, max_noutput_items));
    }

    /*
     * The private constructor
     */
    packetize_burst_impl::packetize_burst_impl(const char* tag, int max_noutput_items)
      : gr::sync_block(
          "packetize_burst",
          gr::io_signature::make(1, 1, sizeof(float)),
          gr::io_signature::make(0, 0, 0)),
	tag_(tag),
	max_noutput_items_(max_noutput_items)
    {
      message_port_register_out(PDU_PORT_ID);
    }

    /*
     * Our virtual destructor.
     */
    packetize_burst_impl::~packetize_burst_impl()
    {
    }

    int
    packetize_burst_impl::work(
        int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float* in = static_cast<const float*>(input_items[0]);

      // For testing: set maximum amount of output to write (and
      // therefore also input to read).
      if (max_noutput_items_ > 0) {
	noutput_items = std::min(max_noutput_items_, noutput_items);
      }

      std::vector<tag_t> tags;
      get_tags_in_window(tags, 0, 0, noutput_items);
      auto relative_offset = nitems_read(0);
      if (debug) {
	std::cerr << "at " << nitems_read(0) << " want " << noutput_items << std::endl;
      }

      // Common case: not capturing packet now, and no tags.
      if (buffer_.empty() && tags.empty()) {
	return noutput_items;
      }

      // API apparently doesn't guarantee tag order.
      std::sort(tags.begin(), tags.end(), tag_t::offset_compare);
      
      for (const auto& t : tags) {
	if (pmt::symbol_to_string(t.key) != tag_) {
	  continue;
	}
	if (pmt::to_bool(t.value)) {
	  if (debug) {
	    std::cerr << "start of packet at abs offset " << t.offset << std::endl;
	  }
	  // Start of packet. Add first sample and handle the rest in "middle of packet".
	  // We could add middle of packet here, but would need to handle the case where
	  // both start of packet and end of packet is in the same window.
	  buffer_.push_back(in[t.offset - relative_offset]);
	  const auto c = t.offset - relative_offset + 1;
	  return c;
	} else {
	  // End of packet.
	  if (debug) {
	    std::cerr << "End of packet at offset " << t.offset << std::endl;
	  }
	  const auto eat_samples = std::min(
            t.offset - relative_offset + 1, static_cast<unsigned long>(noutput_items));
	  if (!buffer_.empty()) {
	    std::copy(&in[0], &in[eat_samples], std::back_inserter(buffer_));
	  }
	  const pmt::pmt_t vecpmt(pmt::make_blob(&buffer_[0], sizeof(float)*buffer_.size()));
	  const pmt::pmt_t pdu(pmt::cons(pmt::PMT_NIL, vecpmt));
	  
	  message_port_pub(PDU_PORT_ID, pdu);
	  if (debug) {
	    std::cerr << "Sent packet of size " << buffer_.size() << std::endl;
	  }
	  buffer_.clear();
	  return eat_samples;
	}
      }

      // No burst tag seen.

      if (!buffer_.empty()) {
	// Middle of packet.
	std::copy(&in[0], &in[noutput_items], std::back_inserter(buffer_));
      }
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
