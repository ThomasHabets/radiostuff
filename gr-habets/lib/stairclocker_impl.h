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

#ifndef INCLUDED_HABETS_STAIRCLOCKER_IMPL_H
#define INCLUDED_HABETS_STAIRCLOCKER_IMPL_H

#include <habets/stairclocker.h>

namespace gr {
  namespace habets {

    class stairclocker_impl : public stairclocker
    {
     private:
      const int min_plateu_;
      const pmt::pmt_t port_in_;
      const pmt::pmt_t port_out_;

      std::vector<float> process(const std::vector<float>& in) const;

     public:
      stairclocker_impl(int min_plateu);
      ~stairclocker_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_STAIRCLOCKER_IMPL_H */

/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=8 sw=8
 */
