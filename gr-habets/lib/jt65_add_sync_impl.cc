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
#include "jt65_add_sync_impl.h"

const int input_size = 63;
const int output_size = 126;
const int symbol_offset = 2;

// At these timeslots send the sync symbol, 1270.5Hz.
const std::vector<bool> sync_pos{
    1,0,0,1,1,0,0,0,1,1,1,1,1,1,0,1,0,1,0,0,0,1,0,1,1,0,0,1,0,0,
    0,1,1,1,0,0,1,1,1,1,0,1,1,0,1,1,1,1,0,0,0,1,1,0,1,0,1,0,1,1,
    0,0,1,1,0,1,0,1,0,1,0,0,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,1,
    0,1,0,0,1,0,1,1,0,1,0,1,0,1,0,0,1,1,0,0,1,0,0,1,0,0,0,0,1,1,
    1,1,1,1,1,1
};


namespace gr {
  namespace habets {

    jt65_add_sync::sptr
    jt65_add_sync::make()
    {
      return gnuradio::get_initial_sptr
        (new jt65_add_sync_impl());
    }

    /*
     * The private constructor
     */
    jt65_add_sync_impl::jt65_add_sync_impl()
      : gr::sync_block("jt65_add_sync",
		       gr::io_signature::make(1, 1, input_size * sizeof(char)),
		       gr::io_signature::make(1, 1, output_size * sizeof(char)))
    {}

    /*
     * Our virtual destructor.
     */
    jt65_add_sync_impl::~jt65_add_sync_impl()
    {
    }

    int
    jt65_add_sync_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char* in = static_cast<const char*>(input_items[0]);
      char* out = static_cast<char*>(output_items[0]);

      for (const auto& t : sync_pos) {
        if (t) {
          *out++ = 0;
        } else {
          *out++ = *in++ + symbol_offset;
        }
      }

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
