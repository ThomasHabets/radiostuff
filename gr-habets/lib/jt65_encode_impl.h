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

#ifndef INCLUDED_HABETS_JT65_ENCODE_IMPL_H
#define INCLUDED_HABETS_JT65_ENCODE_IMPL_H

#include <queue>

#include <habets/jt65_encode.h>

namespace gr {
  namespace habets {

    class jt65_encode_impl : public jt65_encode
    {
     private:
      std::queue<std::string> queue_;

     public:
      jt65_encode_impl();
      ~jt65_encode_impl();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items) override;
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_JT65_ENCODE_IMPL_H */

/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=2 sw=2
 */
