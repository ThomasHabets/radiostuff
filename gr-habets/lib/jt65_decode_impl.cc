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
#include<numeric>
#include<cassert>
#include<gnuradio/fft/fft.h>
#include<fftw3.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <gnuradio/io_signature.h>
#include "jt65_decode_impl.h"
#include "jt65.h"

// TODO: move to JT65 namespace.
const std::vector<bool> sync_pos{
    1,0,0,1,1,0,0,0,1,1, // 0-9
    1,1,1,1,0,1,0,1,0,0, // 10-19
    0,1,0,1,1,0,0,1,0,0, // 20-29
    0,1,1,1,0,0,1,1,1,1, // 30-39
    0,1,1,0,1,1,1,1,0,0, // 40-49
    0,1,1,0,1,0,1,0,1,1, // 50-59
    0,0,1,1,0,1,0,1,0,1, // 70-69
    0,0,1,0,0,0,0,0,0,1, // 80-79
    1,0,0,0,0,0,0,0,1,1, // 90-89
    0,1,0,0,1,0,1,1,0,1, // 100-99
    0,1,0,1,0,0,1,1,0,0, // 110-109
    1,0,0,1,0,0,0,0,1,1, // 120-129
    1,1,1,1,1,1          // 130-136
};


namespace {
  constexpr bool debug = false;
}

//using JT65::sync_pos;
using gr::fft::fft_complex;

namespace gr {
  namespace habets {
    namespace {

      template<typename T>
      std::string
      vector2string(const std::vector<T>& in)
      {
        std::stringstream ss;
        ss << in[0];
        for (int n = 1; n < in.size(); n++) {
          ss << " " << in[n];
        }
        return ss.str();
      }

      class Modder {
      public:
        virtual std::string name() const = 0;
        virtual std::vector<float> operator()(const std::vector<float>&) = 0;
      };

      // Sync up to start of the first bit.
      class Sync : public Modder {
        const int buckets_per_symbol_;
      public:
        std::string name() const override { return "sync"; };

        Sync(int b): buckets_per_symbol_(b){}

        std::vector<float>
        operator()(const std::vector<float>& in) override {
          auto is_sync = [](int s) {
            return s < 5;
          };

          int skip = 0;

          // Skip up to the first few sync bits in a row.
          {
            int seen_sync = 0;
            for (int n = 0; n < in.size(); n++) {
              const auto& s = in[n];
              if (seen_sync > buckets_per_symbol_/2) {
                skip = n - seen_sync;
                assert(skip >= 0);
                break;
              }
              if (is_sync(s)) {
                seen_sync++;
              } else {
                seen_sync = 0;
              }
            }
          }
          if (debug) {
            std::clog << "jt65_decode: skipped " << skip << " to get some syncs syncs\n";
          }

          // Skip so that we have at least a few nonsync.
          {
            int seen_nonsync = 0;
            for (int n = skip; n < in.size(); n++) {
              const auto& s = in[n];
              if (seen_nonsync > buckets_per_symbol_/2) {
                skip = std::max(skip, n - seen_nonsync - buckets_per_symbol_ + 1);
                assert(skip >= 0);
                break;
              }
              if (!is_sync(s)) {
                seen_nonsync++;
              } else {
                seen_nonsync = 0;
              }
            }
          }
          return std::vector<float>(in.begin() + skip, in.end());
        }
      };

      double stddev_and_penalty(const float* from,const float* to, const bool is_sync) {
        double sum = 0;
        double sq_sum = 0;
        int n = 0;
        double penalty = 0;
        for (;from != to;from++) {
          sq_sum += *from * *from;
          sum += *from;
          n++;
          penalty += 100 * ((*from < -1) != is_sync);
        }
        const double mean = sum / n;
        return penalty + sq_sum / n - mean*mean;
      }

      // Attempt to sync symbols so that they land on bucket_per_symbol boundaries.
      class BitSync : public Modder {
        const int buckets_per_symbol_;
      public:
        std::string name() const override { return "bitsync"; };
        BitSync(int b): buckets_per_symbol_(b){}

        std::vector<float>
        operator()(const std::vector<float>& in) {
          std::vector<float> out;
          int pos = 0;
          const int width = 7;
          int total_ofs = 0;
          for (const auto& is_sync : sync_pos) {
            const bool first = out.empty();
            // TODO: this drift only adjusts for non-sync
            // symbols. Ideally it should probably be calculated from
            // output size and how much has been consumed to produce that output.
            const double drift = (double(total_ofs) / (first ? 1 : out.size()));
            if (is_sync) {
              for (int c = 0; c < buckets_per_symbol_; c++) {
                out.push_back(in[pos++]);
              }
              pos += buckets_per_symbol_ * (first ? 0.0 : drift);
              if (debug) {
                std::clog << " sync. next offset: " << pos << std::endl;
              }
              continue;
            }

            int best = 0;
            double best_diff = std::numeric_limits<double>::max();
            std::stringstream scores;
            for (int ofs = -width; ofs <= width; ofs++) {
              const auto& from = &in[pos+ofs];
              const auto& to = &in[pos+ofs+buckets_per_symbol_];

              const auto score = stddev_and_penalty(from, to, is_sync) + std::abs(ofs);
              if (score < best_diff) {
                best = ofs;
                best_diff = score;
              }
              scores << score << " ";
            }
            if (debug) {
              std::clog << "Bitsync symbol " << (out.size()/buckets_per_symbol_)
                        << " drift=" << drift
                        << " best=" << best_diff << " @" << (pos+best)
                        << " range=(" << (pos-width) << "-" << (pos+width) << ")"
                        << " all scores: " << scores.str() << std::endl;
            }
            for (int c = 0; c < buckets_per_symbol_; c++) {
              out.push_back(in[pos+best+c]);
            }
            pos += best + buckets_per_symbol_ * (1.0 + drift);
            if (debug) {
              std::clog << " next offset: " << pos << std::endl;
            }
            total_ofs += best;
          }
          return out;
        }
      };

      class RemoveSync : public Modder {
        const int buckets_per_symbol_;
      public:
        std::string name() const override { return "removesync"; };
        RemoveSync(int b): buckets_per_symbol_(b) {}
        std::vector<float>
        operator()(const std::vector<float>& in) {
          std::vector<float> out;
          if (true) {
            const double default_sync = -2.0;
            double sync_adjust = 0;
            for (int pos = 0; pos < in.size(); pos++) {
              const bool is_sync = sync_pos[pos];
               // in[pos] to take care of overlong syncs.
              //if ((is_sync && in[pos] < 10) || in[pos] < 0) {
              if (is_sync) {// || in[pos] < 0) {
                const auto& ofs = in[pos] - default_sync + sync_adjust;
                sync_adjust += -0.2*ofs;
                if (debug) { std::clog << "removesync: @" << pos << "=" << sync_adjust << " after " << (-0.1*ofs) << " adj" << std::endl; }
                continue;
              }
              if (debug) { std::clog << "removesync-adj: @" << pos << " " << in[pos] << " -> " << (in[pos]+sync_adjust) << std::endl; }
              out.push_back(in[pos] + sync_adjust);
            }
          } else {
            // TODO: confirm sync.
            if (true) {
              for (const auto& v : in) {
                if (v >= 0) {
                  out.push_back(v);
                }
              }
            } else {
              for (int i = 0; i < in.size(); i++) {
                if (!sync_pos[i / buckets_per_symbol_]) {
                  out.push_back(in[i]);
                }
              }
            }
          }
          return out;
        }
      };

      // Pick median value.
      class Pick : public Modder {
        const int buckets_per_symbol_;
      public:
        std::string name() const override { return "pick"; };
        Pick(int b): buckets_per_symbol_(b){}
        std::vector<float>
        operator()(const std::vector<float>& in_) {
          std::vector<float> out;
          std::vector<float> in(in_);
          const int width = 0.5 * buckets_per_symbol_;
          for (int pos = buckets_per_symbol_/2; pos < in.size();) {
            const bool is_sync = sync_pos[out.size()/buckets_per_symbol_];
            const auto& frompos = pos-width+1;
            const auto& topos = pos+width;
            const auto& from = &in[frompos];
            const auto& to = &in[topos];
            std::nth_element(from, &in[pos], to);
            const auto& v = in[pos];
            if (debug) {
              std::clog << "for " << out.size() << " picking " << frompos << " - " << topos << " value " << v << std::endl;
            }
            out.push_back(v);
            pos += buckets_per_symbol_;
          }
          return out;
        }
      };

      // Scale from bucket index to symbol.
      class Scale : public Modder {
        const int samp_rate_;
        const int fft_size_;
        const float symbol_offset_;
      public:
        std::string name() const override { return "scale"; };
        Scale(int samp_rate, int fft_size, float symbol_offset): samp_rate_(samp_rate), fft_size_(fft_size), symbol_offset_(symbol_offset) {}
        std::vector<float>
        operator()(const std::vector<float>& in) {
          const float m = symbol_offset_ * 64 * fft_size_ / samp_rate_;
          auto out = in;
          if (debug) { std::clog << "jt65_decode: scaler: " << m << std::endl; }
          for (auto& o : out) {
            o = 64.0*o/m - 2;
          }
          return out;
        }
      };

      // Bucket-translate according to what the base is.
      class AdjustBase : public Modder {
      public:
        std::string name() const override { return "adjustbase"; };
        std::vector<float>
        operator()(const std::vector<float>& in) {
          std::map<float,int> counts;
          int mx = -1;
          float mxval = -1;
          for (const auto& v : in) {
            const auto t = ++counts[v];
            if (t > mx) {
              mx = t;
              mxval = v;
            }
          }
          if (debug) {
            std::clog << "jt65_decode: base is " << mxval << std::endl;
          }
          auto out = in;
          for (auto& o : out) {
            o -= mxval;
          }
          return out;
        }
      };

      // Write raw data to file.
      void
      dump(const std::vector<float>& vs, const std::string& fn) {
        std::ofstream of(fn);
        for (const auto& v : vs) {
          of << v << std::endl;
        }
      }

      std::vector<float>
      runfft(const std::vector<float>& fs, const int batch, const int fft_size) {
        // std::clog << "runfft on " << fs.size() << std::endl;
        const bool forward = true;

        std::vector<float> buckets;
        buckets.reserve(fs.size() / batch);
        for (int pos = 0; pos < fs.size(); pos += batch) {
          // Set up FFT buffer.
          std::vector<float> buf(fft_size);
          const int end = pos + std::min(batch, int(fs.size() - pos));
          std::copy(&fs[pos], &fs[end], buf.begin());
          auto fft = std::make_unique<fft_complex>(fft_size, forward, 4);

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
    jt65_decode::make(int samp_rate, int sps, int buckets_per_symbol, int fft_size, float symbol_offset)
    {
      return gnuradio::get_initial_sptr
        (new jt65_decode_impl(samp_rate, sps, buckets_per_symbol, fft_size, symbol_offset));
    }

    /*
     * The private constructor
     */
    jt65_decode_impl::jt65_decode_impl(int samp_rate, int sps, int buckets_per_symbol, int fft_size, float symbol_offset)
      : gr::block("jt65_decode",
                  gr::io_signature::make(0,0,0),
                  gr::io_signature::make(0,0,0)),
        samp_rate_(samp_rate),
        buckets_per_symbol_(buckets_per_symbol),
        fft_size_(fft_size),
        sps_(sps),
        symbol_offset_(symbol_offset),
        batch_(sps/buckets_per_symbol)
    {
      message_port_register_in(pmt::intern("in"));
      set_msg_handler(pmt::intern("in"), [this](pmt::pmt_t msg) {
          if (debug) {
            std::clog << "C++> got message\n";
          }
          pmt::pmt_t meta = pmt::car(msg);
          pmt::pmt_t data = pmt::cdr(msg);
          const size_t len = pmt::blob_length(data);

          if (debug) {
            std::clog << "C++> size " << len << std::endl;
          }
          auto fs = static_cast<const float*>(pmt::blob_data(data));

          const auto out = decode(std::vector<float>(&fs[0], &fs[len/sizeof(float)]));
          if (debug) {
            std::clog << "jt65_decode: C++ decoded " << out.size() << std::endl;
          }
          const pmt::pmt_t vecpmt(pmt::make_blob(out.data(), out.size()));
          const pmt::pmt_t pdu(pmt::cons(meta, vecpmt));
          message_port_pub(pmt::intern("out"), pdu);
      });
      message_port_register_out(pmt::intern("out"));
    }

    std::string
    jt65_decode_impl::decode(const std::vector<float>&fs) const {
      if (debug) {
        std::clog << "jt65_decode: decoding with " << fs.size() << " floats\n";
      }
      auto buckets = runfft(fs, batch_, fft_size_);

      if (debug) { dump(buckets, "di.runfft"); }
      std::vector<std::unique_ptr<Modder>> modders;
      modders.push_back(std::make_unique<AdjustBase>());
      modders.push_back(std::make_unique<Sync>(buckets_per_symbol_));
      modders.push_back(std::make_unique<Scale>(samp_rate_, fft_size_, symbol_offset_));
      modders.push_back(std::make_unique<BitSync>(buckets_per_symbol_));
      modders.push_back(std::make_unique<Pick>(buckets_per_symbol_));
      modders.push_back(std::make_unique<RemoveSync>(buckets_per_symbol_));

      for (const auto& mod : modders) {
        buckets = (*mod)(buckets);
        if (debug) {
          dump(buckets, "di."+mod->name());
        }
      }
      std::vector<int> syms;
      for (const auto& f :buckets) {
        syms.push_back(std::min(63, std::max(0, int(std::roundf(f)))));
      }

      if (debug) {
        std::clog << "jt65_decode: unpacking " << syms.size() << " symbols…\n"
                  << vector2string(syms) << std::endl;
      }
      if (syms.size() == 62) {
        // TODO: this symbol is most likely lost somewhere above, and should be reclaimed.
        buckets.push_back(0);
      } else if (syms.size() < 63) {
        std::clog << "invalid decode. Only got " << std::to_string(syms.size())
                  << " symbols: " << vector2string(syms) << std::endl;;
        return  "invalid decode. Only got " + std::to_string(syms.size());
      }
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
