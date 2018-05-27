/* -*- c++ -*- */
/* 
 * Copyright 2018 <+YOU OR YOUR COMPANY+>.
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

#include "jt65_encode_impl.h"
#include "jt65.h"

const int output_size = 63;

namespace gr {
  namespace habets {

    jt65_encode::sptr
    jt65_encode::make()
    {
      return gnuradio::get_initial_sptr
        (new jt65_encode_impl());
    }

    /*
     * The private constructor
     */
    jt65_encode_impl::jt65_encode_impl()
      : gr::sync_block("jt65_encode",
                       gr::io_signature::make(0, 0, 0),
                       gr::io_signature::make(1, 1, output_size * sizeof(char)))
    {
      message_port_register_in(PDU_PORT_ID);
      set_msg_handler(PDU_PORT_ID, [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);
          const uint8_t* bytes = static_cast<const uint8_t*>(pmt::blob_data(data));
          const std::string o(bytes, bytes+len);
          //std::cout << "got string: " << o << std::endl;
          queue_.push(o);
      });
    }

    /*
     * Our virtual destructor.
     */
    jt65_encode_impl::~jt65_encode_impl()
    {
    }

    int
    jt65_encode_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      if (queue_.empty()) {
        usleep(100); // TODO: find a better way to not go 100% CPU.
        return 0;
      }

      char *out = static_cast<char*>(output_items[0]);
      const auto msg = queue_.front();
      const auto enc = JT65::greycode(JT65::interleave(JT65::fec(JT65::pack_message(msg))));
      for (int i = 0; i < output_size; i++) {
        //std::cout << " " << enc[i];
        out[i] = enc[i];
      }
      queue_.pop();
      return 1;
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
