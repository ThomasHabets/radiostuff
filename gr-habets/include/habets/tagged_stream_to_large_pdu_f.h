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


#ifndef INCLUDED_HABETS_TAGGED_STREAM_TO_LARGE_PDU_F_H
#define INCLUDED_HABETS_TAGGED_STREAM_TO_LARGE_PDU_F_H

#include <habets/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace habets {

    /*!
     * \brief <+description of block+>
     * \ingroup habets
     *
     */
    class HABETS_API tagged_stream_to_large_pdu_f : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<tagged_stream_to_large_pdu_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of habets::tagged_stream_to_large_pdu_f.
       *
       * To avoid accidental use of raw pointers, habets::tagged_stream_to_large_pdu_f's
       * constructor is in a private implementation
       * class. habets::tagged_stream_to_large_pdu_f::make is the public interface for
       * creating new instances.
       */
      static sptr make(const std::string& length_tag);
    };

  } // namespace habets
} // namespace gr

#endif /* INCLUDED_HABETS_TAGGED_STREAM_TO_LARGE_PDU_F_H */
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=2 sw=2
 */
