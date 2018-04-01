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

#include<numeric>

#include <gnuradio/blocks/pdu.h>
#include <gnuradio/io_signature.h>
#include "stairclocker_impl.h"

namespace gr {
  namespace habets {

    bool
    off(float a, float b, float percent)
    {
      return fabs(a - b) > 0.02;
      if (a == 0.0) {
        if (b == 0.0) {
          return 0.0;
        }
        float t = a;
        a = b;
        b = t;
      }
      return fabs((a - b) / a) > (percent / 100.0);
    }

    stairclocker::sptr
    stairclocker::make(int min_size)
    {
      return gnuradio::get_initial_sptr(new stairclocker_impl(min_size));
    }

    /*
     * The private constructor
     */
    stairclocker_impl::stairclocker_impl(int min_size)
      : gr::block("stairclocker",
		  gr::io_signature::make(0,0,0),
		  gr::io_signature::make(0,0,0)),
	min_plateu_(min_size),
        port_in_(pmt::intern("in")),
        port_out_(pmt::intern("out"))
    {
      message_port_register_in(port_in_);
      set_msg_handler(port_in_, [this](pmt::pmt_t msg) {
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data) / sizeof(float);
          const float* in = static_cast<const float*>(pmt::blob_data(data));
          const std::vector<float> out = process(std::vector<float>(&in[0], &in[len]));
          const pmt::pmt_t vecpmt(pmt::make_blob(&out[0], out.size() * sizeof(float)));
          const pmt::pmt_t pdu(pmt::cons(meta, vecpmt));
          message_port_pub(port_out_, pdu);
      });
      message_port_register_out(port_out_);
    }

    std::vector<float>
    stairclocker_impl::process(const std::vector<float>& in) const
    {
      const int max_shortsamples = 3;
      const float off_percent = 20.0;
      
      if (in.size() == 0) {
        //std::clog << "No stairs in zero-length packet\n";
        return std::vector<float>();
      }
      std::clog << "Processing " << in.size() << " length packet\n";
      size_t pos = 0;
      float last = in[pos++];
      size_t match = 0;

      // Strip preamble.
      for (; pos < in.size(); pos++) {
        if (off(last, in[pos], off_percent)) {
          std::clog <<  "Discarding " << in[pos] << " for being far from " << last << std::endl;
          last = in[pos];
          match = 0;
          continue;
        }
        match++;
        if (match == min_plateu_) {
          pos -= match;
          break;
        }
      }
      
      // No stairs.
      if (pos == in.size()) {
        //std::clog << "No stairs\n";
        return std::vector<float>();
      }

      std::clog << "Stairs starting at pos: " << pos << std::endl;

      // Record stairsamples.
      
      int shortsamples = 0;
      std::vector<std::vector<float> > stairsamples;
      std::vector<float> candidate{in[pos++]};
      float candidate_sum = candidate[0];
      
      auto new_plateu = [&]{
          candidate.clear();
          candidate.push_back(in[pos]);
          candidate_sum = candidate[0];
          //std::clog << "New plateu: " << candidate[0] << std::endl;
      };
      for (; pos < in.size(); pos++) {
        if (off(candidate_sum / candidate.size(), in[pos], off_percent)) {
          if (candidate.size() < min_plateu_) {
            if (shortsamples > max_shortsamples) {
              std::clog << "Short candidate: " << candidate.size() << " Stairs ending at pos: " << pos << std::endl;
              break;
            } else {
              std::clog << "  Never mind, that was a shortsample of size " << candidate.size() << " " << candidate_sum / candidate.size() << std::endl;
              std::clog << "  Adding sample " << in[pos] << std::endl;
              shortsamples++;
              new_plateu();
              continue;
            }
          }
          shortsamples = 0;
          std::clog << "Commit plateu of size " << candidate.size() << std::endl;
          stairsamples.push_back(candidate);
          new_plateu();
        } else {
          std::clog << "Adding sample " << in[pos] << std::endl;
          candidate.push_back(in[pos]);
          candidate_sum += in[pos];
        }
      }
      if (candidate.size() >= min_plateu_) {
        stairsamples.push_back(candidate);
      }

      std::clog << "Stair size: " << stairsamples.size() << std::endl;

      // Find actual stairsize.
      size_t shortest_step = in.size();
      for (const auto& s: stairsamples) {
        shortest_step = std::min(s.size(), shortest_step);
      }

      std::clog << "Step size: " << shortest_step << std::endl;

      std::vector<float> ret;
      std::clog << "Complete stairs\n";
      for (const auto& s: stairsamples) {
        const float sum = std::accumulate(s.begin(), s.end(), 0.0);
        ret.insert(ret.end(), s.size() / shortest_step, sum / s.size());
        printf(" %d %f\n", s.size() / shortest_step, sum / s.size());
      }
      return ret;
    }

    /*
     * Our virtual destructor.
     */
    stairclocker_impl::~stairclocker_impl()
    {
    }

    void
    stairclocker_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    stairclocker_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
      // Will never be called.
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
 * vim: ts=8 sw=8
 */
