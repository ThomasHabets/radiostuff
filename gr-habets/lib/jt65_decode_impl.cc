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
#include "jt65_decode_impl.h"

namespace gr {
  namespace habets {

    jt65_decode::sptr
    jt65_decode::make()
    {
      return gnuradio::get_initial_sptr
        (new jt65_decode_impl());
    }

    /*
     * The private constructor
     */
    jt65_decode_impl::jt65_decode_impl()
      : gr::block("jt65_decode",
                  gr::io_signature::make(0,0,0),
                  gr::io_signature::make(0,0,0))
    {
      message_port_register_in(pmt::intern("in"));
      set_msg_handler(pmt::intern("in"), [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);
          auto fs = static_cast<const float*>(pmt::blob_data(data));
          const auto out = decode(std::vector<float>(&fs[0], &fs[len]));
          const pmt::pmt_t vecpmt(pmt::make_blob(out.data(), out.size()));
          const pmt::pmt_t pdu(pmt::cons(meta, vecpmt));
          message_port_pub(pmt::intern("out"), pdu);
      });
      message_port_register_out(pmt::intern("out"));
    }

    std::string
    jt65_decode_impl::decode(const std::vector<float>&fs) const {
      return "TODO";
    }

    /*
     * Our virtual destructor.
     */
    jt65_decode_impl::~jt65_decode_impl()
    {
    }

    void
    jt65_decode_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    jt65_decode_impl::general_work (int noutput_items,
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
 * vim: ts=2 sw=2
 */
