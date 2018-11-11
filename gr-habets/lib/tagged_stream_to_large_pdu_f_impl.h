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

#ifndef INCLUDED_HABETS_TAGGED_STREAM_TO_LARGE_PDU_F_IMPL_H
#define INCLUDED_HABETS_TAGGED_STREAM_TO_LARGE_PDU_F_IMPL_H

#include <string>
#include <pmt/pmt.h>
#include <habets/tagged_stream_to_large_pdu_f.h>

namespace gr {
  namespace habets {

    class tagged_stream_to_large_pdu_f_impl : public tagged_stream_to_large_pdu_f
    {
     private:
      const std::string length_tag_;
      size_t current_length_ = 0;
      std::vector<float> buffer_;
      const pmt::pmt_t pdu_port_id_ = pmt::intern("out");

     public:
      tagged_stream_to_large_pdu_f_impl(const std::string& length_tag);
      ~tagged_stream_to_large_pdu_f_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_TAGGED_STREAM_TO_LARGE_PDU_F_IMPL_H */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=2 sw=2
 */
