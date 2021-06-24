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
        outs[n] << tod << " " << (st.sum / st.count) << " " << (st.count / st.days_seen)
                << "\n";
    }
}


int main(int argc, char** argv)
{
    std::ios_base::sync_with_stdio(false);
    assert("IO91" == maidenhead_from_index(maidenhead_to_index("IO91")));

    const std::vector<int> bands{ 20, 40 };
    std::vector<std::vector<DistState>> dist_states;
    auto state_reset = [&dist_states, &bands] {
        dist_states.clear();
        for (long unsigned int b = 0; b < bands.size(); b++) {
            dist_states.emplace_back(maiden_count);
        }
    };
    state_reset();

    // Open output files.
    std::vector<std::vector<std::ofstream>> outs;
    for (const auto band : bands) {
        std::vector<std::ofstream> t;
        for (int n = 0; n < maiden_count; n++) {
            auto f = std::ofstream("out.distday/" + maidenhead_from_index(n) + "." +
                                   std::to_string(band));
            if (!f.good()) {
                throw std::runtime_error(std::string("failed to open output file: ") +
                                         strerror(errno));
            }
            t.push_back(std::move(f));
        }
        outs.push_back(std::move(t));
    }

    int cur_tod = -1;
    for (auto line : StreamLines(std::cin)) {
        // std::cerr << "LINE: <" << line << ">\n";
        const auto cols = split<7>(line);
        if (cols[0].empty()) {
            // Splitting failed.
            std::cerr << "Split failed\n";
            continue;
        }
        auto& src = cols[5];
        auto& dst = cols[6];
        if (dst.size() < 4) {
            continue;
        }
        if (src.size() < 4) {
            continue;
        }
        const auto band = parse_band(cols[2]);
        if (band == -1) {
            // Unknown band.
            continue;
        }
        auto& mode = cols[3];
        if (mode != "FT8") {
            continue;
        }
        const std::string_view dst4(dst.begin(), dst.begin() + 4);
        const std::string_view src4(src.begin(), src.begin() + 4);
        const auto dstindex = maidenhead_to_index(dst4);
        const auto srcindex = maidenhead_to_index(src4);
        // const auto tod = ((parse_int(cols[0])%86400) / period) * period;
        const auto tod = parse_int(cols[0]);
        const auto day = parse_int(cols[1]) / 86400; // TODO: parse_int64
        const auto coord_src = decode_maidenhead(src);
        const auto coord_dst = decode_maidenhead(dst);
        if (cur_tod != tod) {
            for (size_t oband = 0; oband < bands.size(); oband++) {
                print_lines(cur_tod, outs[oband], dist_states[oband]);
            }
            state_reset();
            cur_tod = tod;
        }
        for (auto index : std::array<int, 2>{ dstindex, srcindex }) {
            auto& dist_entry = dist_states[band][index];
            dist_entry.sum += dist(coord_src, coord_dst);
            if (dist_entry.last_day_seen != day) {
                dist_entry.days_seen++;
                dist_entry.last_day_seen = day;
            }
            dist_entry.count++;
        }
        // std::cout << line << " " << band << "\n";
    }

    std::cerr << "todistday: Flushing and closing output files…\n";
    ThreadPool pool(get_nprocs() * 20);
    // std::vector<std::jthread> threads;
    for (auto& os : outs) {
        for (auto& out : os) {
            pool.add([&out] {
                out.close();
                if (out.fail()) {
                    throw std::runtime_error(
                        std::string("Failed to flush/close output file: ") +
                        strerror(errno));
                }
            });
        }
    }
}