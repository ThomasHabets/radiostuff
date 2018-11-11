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
#include<algorithm>
#include<gnuradio/fft/fft.h>
#include<fftw3.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gnuradio/io_signature.h>
#include "jt65_decode_impl.h"
#include "jt65.h"

using gr::fft::fft_complex;

namespace gr {
  namespace habets {
    namespace {

      std::vector<int>
      remove_sync(const std::vector<int>& in) {
        // TODO: confirm sync.
        std::vector<int> out;
        for (const auto& v : in) {
          if (v < 0) {
              continue;
          }
          out.push_back(v);
        }
        return out;
      }

      std::vector<int>
      pick(const std::vector<int>& in, const int sps) {
        std::vector<int> out;
        for (int pos = sps/2; pos < in.size(); pos += sps) {
          out.push_back(in[pos]);
        }
        return out;
      }

      std::vector<int>
      scale(const std::vector<int>& in, const int samp_rate, const int fft_size) {
        // JT65C 10.8Hz per level.
        const float m = 10.8 * 64 * fft_size / samp_rate;
        auto out = in;
        std::clog << "scaler: " << m << std::endl;
        for (auto& o : out) {
          o = std::roundf(o*64.0/m) - 2;
        }
        return out;
      }

      std::vector<int>
      adjust_base(const std::vector<int>& in) {
        std::map<int,int> counts;
        int mx = -1;
        int mxval = -1;
        for (const auto& v : in) {
          auto t = ++counts[v];
          if (t > mx) {
            mx = t;
            mxval = v;
          }
        }
        auto out = in;
        for (auto& o : out) {
          o -= mxval;
        }
        return out;
      }

      void
      dump(const std::vector<int>& vs) {
        std::ofstream of("di");
        for (const auto& v : vs) {
          of << v << std::endl;
        }
      }

      std::vector<int>
      runfft(const std::vector<float>& fs, const int batch, const int fft_size) {
        std::clog << "runfft on " << fs.size() << std::endl;
        const bool forward = true;

        std::vector<int> buckets;
        buckets.reserve(fs.size() / batch);
        for (int pos = 0; pos < fs.size(); pos += batch) {
          // Set up FFT buffer.
          std::vector<float> buf(fft_size);
          const int end = pos + std::min(batch, int(fs.size() - pos));
          std::copy(&fs[pos], &fs[end], buf.begin());
          auto fft = std::make_unique<fft_complex>(fft_size, forward, 1);

          gr_complex* dst = fft->get_inbuf();
          for (unsigned int i = 0; i < fft_size; i++) {   // float to complex conversion
            dst[i] = 0;
          }
          for (unsigned int i = 0; i < buf.size(); i++) {   // float to complex conversion
            dst[i] = buf[i];
          }

          fft->execute();
          dst = fft->get_outbuf();
          // Find biggest bucket.
          float curmax = std::abs(dst[0]);
          int max_index = 0;
          for (int i = 1; i < fft_size/2; i++) {
            const auto t = std::abs(dst[i]);
            if (curmax < t) {
              max_index = i;
              curmax = t;
            }
          }
          max_index -= 2;
          //std::clog << "Max bucket: " << max_index << " " << curmax << std::endl;

          buckets.push_back(max_index);
        }
        return buckets;
      }
    }

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
          std::clog << "C++> got message\n";
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);

          std::clog << "C++> size " << len << std::endl;
          auto fs = static_cast<const float*>(pmt::blob_data(data));

          const auto out = decode(std::vector<float>(&fs[0], &fs[len/sizeof(float)]));
          const pmt::pmt_t vecpmt(pmt::make_blob(out.data(), out.size()));
          const pmt::pmt_t pdu(pmt::cons(meta, vecpmt));
          message_port_pub(pmt::intern("out"), pdu);
      });
      message_port_register_out(pmt::intern("out"));
    }

    std::string
    jt65_decode_impl::decode(const std::vector<float>&fs) const {
      std::clog << "C++ decoding with " << fs.size() << " floats\n";
      const auto buckets = runfft(fs, batch_, fft_size_);
      const auto based = adjust_base(buckets);
      const auto synced = based; // TODO
      const auto scaled = scale(synced, samp_rate_, fft_size_);
      const auto picked = pick(scaled, buckets_per_symbol_);
      const auto syms = remove_sync(picked);
      //dump(synced);
      return JT65::unpack_message(JT65::unfec(JT65::uninterleave(JT65::ungreycode(syms))));
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
