// INPUT: CSV with:
// tod,epoch,freq,mode,SNR,fromGrid,toGrid
// sorted by tod,epoch
//
// Improvement ideas:
// * band map -> vector
// * rethink write path
// * background state flusher?
// * pure memset state flusher?

#include "lib.h"
#include "lines.h"
#include <cstring>
#include <fstream>
#include <map>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

int parse_band(std::string_view s)
{
    const auto i = parse_int(s);
    if (i > 7000000 && i < 7300000) {
        return 1;
    }
    if (i > 14000000 && i < 14350000) {
        return 0;
    }
    return -1;
}

struct DistState {
    double sum = 0;
    int count = 0;
    int days_seen = 0;
    int last_day_seen = 0;
};

void print_lines(int tod,
                 std::vector<std::ofstream>& outs,
                 const std::vector<DistState>& states)
{
    for (size_t n = 0; n < states.size(); n++) {
        const auto& st = states[n];
        if (!st.count) {
            continue;
        }
        outs[n] << tod << " " << (st.sum / st.count) << " " << "\n";
    }
}

struct State {
  double sum = 0.0;
  int count = 0;
};

int main(int argc, char** argv)
{
  constexpr int period = 15;
    std::ios_base::sync_with_stdio(false);
    assert("IO91" == maidenhead_from_index(maidenhead_to_index("IO91")));

    std::string outbasedir = argv[1];
    
    const std::vector<int> bands{ 20, 40 };

    // band_index -> coord_index -> time_index
    std::vector<std::vector<std::vector<State>>> state(bands.size());
    for (auto& o : state) {
      o.resize(maiden_count);
      for (int n = 0; n < maiden_count; n++) {
	o[n].resize(86400/15);
      }
    }
    mkdir(outbasedir.c_str(), 0755);
    for (auto line : StreamLines(std::cin)) {
        const auto cols = split<6>(line);
        if (cols[0].empty()) {
            // Splitting failed.
	  std::cerr << "Split failed on " << line << "\n";
            continue;
        }
        auto& src = cols[4];
        auto& dst = cols[5];
        if (dst.size() < 4) {
            continue;
        }
        if (src.size() < 4) {
            continue;
        }
        const auto band = parse_band(cols[1]);
        if (band == -1) {
            // Unknown band.
            continue;
        }
        auto& mode = cols[2];
        if (mode != "FT8") {
            continue;
        }
        const std::string_view dst4(dst.begin(), dst.begin() + 4);
        const std::string_view src4(src.begin(), src.begin() + 4);
        const auto dstindex = maidenhead_to_index(dst4);
        const auto srcindex = maidenhead_to_index(src4);
         const auto tod = ((parse_int(cols[0])%86400) / period) * period;
	 //const auto tod = parse_int(cols[0]);
        //const auto day = parse_int(cols[1]) / 86400; // TODO: parse_int64
        const auto coord_src = decode_maidenhead(src);
        const auto coord_dst = decode_maidenhead(dst);

	const auto distance = dist(coord_src, coord_dst);
        for (const auto index : std::array<int, 2>{ dstindex, srcindex }) {
	  auto& entry = state[band][index][tod];
	  entry.count++;
	  entry.sum += distance;
	}
    }
    for (size_t bi = 0; bi < bands.size(); bi++) {
      for (int n = 0; n < maiden_count; n++) {
	const auto& entries = state[bi][n];
	if (!std::any_of(entries.begin(), entries.end(), [](const auto& e){return e.count;})) {
	  continue;
	}
	std::ofstream f(outbasedir + "/" + maidenhead_from_index(n) + "." +
			       std::to_string(bands[bi]));
	for (int t = 0; t < 86400 / 15; t++) {
	  const auto avg = entries[t].count ? (entries[t].sum / entries[t].count) : 0;
	  f << (t*15) << " " << avg << "\n";
	}
	if (!f.good()) {
	  throw std::runtime_error(std::string("failed to open output file: ") +
				   strerror(errno));
	}
      }
    }
}
