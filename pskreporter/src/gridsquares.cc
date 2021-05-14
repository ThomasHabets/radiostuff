// Shellscript: ~12h
// Previous version shelling out: 50min
// PNG C++ version: 8min
// Switch to JPG with quality 100: 1m15s
// * not full core saturation due to regex parsing being the limiting factor on 12 threads
// * quality 50: 50s
// * less copying: 35s
// * threadpool with affinity: 29s (parallelism 11.349)
// * FDO: 28s
//    * ~10% down on instructions & task-clock by `perf stat`
//    * branch misses up by percentage, but because of 30% fewer branches
//
// TODO:
// * automatically select number of threads
// * two-level threadpool queue
// * command line flags
// * exponential decay for grid
// * try performance of other image formats
// * try performance of other video formats
//
// FDO:
//    -fprofile-generate=profile-dir
//    run
//    -fprofile-use=profile-dir -fprofile-correction
//    -Ofast
//
// Input format: snr,ts,msg
#include "lines.h"

#include <fcntl.h>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <regex>
#include <thread>
#include <vector>

#include <jpeglib.h>
#include <png++/png.hpp>

#include "lib.h"

constexpr auto min_maiden_signals = 2;
constexpr auto jpeg_quality = 50;
using floating = float;

[[nodiscard]] std::string_view map_data()
{
    const char* fn = "animdata2.txt";
    int fd = open(fn, O_RDONLY);
    if (!fd) {
        throw std::runtime_error("failed to open");
    }
    struct stat st {
    };
    if (fstat(fd, &st)) {
        close(fd);
        throw std::runtime_error("failed to stat");
    }
    void* data = mmap(
        nullptr, st.st_size, PROT_READ, MAP_SHARED | MAP_NONBLOCK | MAP_POPULATE, fd, 0);
    if (!data) {
        close(fd);
        throw std::runtime_error("failed to mmap");
    }
    close(fd);
    return {
        static_cast<char*>(data),
        static_cast<size_t>(st.st_size),
    };
}

static const std::string call = "[a-zA-Z0-9]{1,3}[0123456789][a-zA-Z0-9]{0,3}[a-zA-Z]";
static const std::string grid = "[A-R][A-R]\\d{2}";

// The order here matters. This order seems to be faster.
static const std::vector<std::regex> res = {
    std::regex("^" + call + " " + call + " (" + grid + ")$"),
    std::regex("^CQ " + call + " (" + grid + ")$"),
};

[[nodiscard]] std::pair<bool, std::string_view> get_maiden(std::string_view msg)
{
    for (const auto& re : res) {
        std::match_results<std::string_view::const_iterator> m;
        if (std::regex_match(msg.begin(), msg.end(), m, re)) {
            std::csub_match match = m[1];
            return { true, { match.first, match.second } };
        }
    }
    return { false, "" };
}

// from index, y x
[[nodiscard]] constexpr std::pair<int, int> maiden_coords(int index)
{
    return { index % 180, index / 180 };
}

using color_t = std::array<unsigned char, 3>;
[[nodiscard]] color_t color(float val)
{
    if (val < 0) {
        val = 0;
    } else if (val >= 1) {
        val = 0.999999;
    } else if (!std::isnormal(val)) {
        val = 0;
    }
    // std::cerr << val << "\n";
    std::array<color_t, 5> colors{
        color_t{ 0, 0, 255 },   color_t{ 0, 255, 255 }, color_t{ 0, 255, 0 },
        color_t{ 255, 255, 0 }, color_t{ 255, 0, 0 },
    };
    val = val * (colors.size() - 1);
    const int idx1 = std::floor(val);
    const auto idx2 = idx1 + 1;
    // assert(idx2 < colors.size());
    const float frac = val - idx1;
    color_t ret;
    for (size_t i = 0; i < ret.size(); i++) {
        ret[i] = static_cast<unsigned char>((colors[idx2][i] - colors[idx1][i]) * frac +
                                            colors[idx1][i]);
    }
    return ret;
}

void create_frame(int frame,
                  std::vector<floating>&& snr_sums,
                  std::vector<int> count,
                  png::image<png::rgb_pixel> image,
                  int w,
                  int h,
                  int blockh,
                  int blockw)
{
    // Make averages.
    std::vector<floating> snr_avgs(maiden_count);
    for (int c = 0; c < maiden_count; c++) {
        if (count[c] < min_maiden_signals) {
            continue;
        }
        snr_avgs[c] = snr_sums[c] / count[c];
    }
    const auto mm = std::minmax_element(snr_avgs.begin(), snr_avgs.end());
    const auto min = *mm.first;
    const auto max = *mm.second;

    // Normalize averages.
    for (auto& avg : snr_avgs) {
        const auto before = avg;
        avg = (avg - min) / (max - min);
        if (avg < 0 || avg > 1) {
            std::cerr << "What? avg not 0-1? " << before << " " << min << " " << max
                      << "\n";
            throw std::runtime_error("failed precondition!");
        }
    }

    // Draw onto png.
    for (int c = 0; c < maiden_count; c++) {
        if (count[c] < min_maiden_signals) {
            continue;
        }
        const auto coords = maiden_coords(c);
        const auto sy = (coords.first * h) / 180;
        const auto sx = (coords.second * w) / 180;
        const auto col = color(snr_avgs[c]);
        const auto pixel = png::rgb_pixel(col[0], col[1], col[2]);
        for (int y = sy; y < sy + blockh; y++) {
            auto& line = image[h - y];

            if constexpr (false) {
                // Shade of red..
                for (int x = sx; x < sx + blockw; x++) {
                    line[x].red = snr_avgs[c] * 255;
                }
            } else if constexpr (false) {
                // No alpha.
                std::fill(&line[sx], &line[sx + blockw], pixel);
            } else {
                // Alpha blend.
                for (int x = sx; x < sx + blockw; x++) {
                    constexpr float blend = 0.5;
                    line[x].red = line[x].red * blend + col[0] * (1 - blend);
                    line[x].green = line[x].green * blend + col[1] * (1 - blend);
                    line[x].blue = line[x].blue * blend + col[2] * (1 - blend);
                }
            }
        }
    }

    // Save PNG.
    // std::format not yet supported by my
    if constexpr (false) {
        char buf[1024];
        snprintf(buf, sizeof buf, "data/blah-v2-%06d.png", frame);
        image.write(buf);
    } else if constexpr (true) { // JPG
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

        char buf[1024];
        snprintf(buf, sizeof buf, "data/blah-v2-%06d.jpg", frame);
        FILE* f = fopen(buf, "wb");
        assert(f);

        jpeg_stdio_dest(&cinfo, f);
        cinfo.image_width = w;
        cinfo.image_height = h;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, jpeg_quality, TRUE);
        jpeg_start_compress(&cinfo, TRUE);
        for (int y = 0; y < h; y++) {
            auto data = reinterpret_cast<const unsigned char*>(image[y].data());
            jpeg_write_scanlines(&cinfo, const_cast<unsigned char**>(&data), 1);
        }
        jpeg_finish_compress(&cinfo);
        fclose(f);
        jpeg_destroy_compress(&cinfo);
    } else {
        // Theoretical lower bound.
        char buf[1024];
        snprintf(buf, sizeof buf, "data/blah-v2-%06d.raw", frame);
        FILE* f = fopen(buf, "wb");
        assert(f);
        for (int y = 0; y < h; y++) {
            fwrite(image[y].data(), w * sizeof(image[y][0]), 1, f);
            break;
        }
        fclose(f);
    }
}

int main(int argc, char** argv)
{
    const auto data = map_data();
    std::ios_base::sync_with_stdio(false);

    const png::image<png::rgb_pixel> image("Maidenhead_Locator_Map.png");
    const auto w = image.get_width();
    const auto h = image.get_height();
    const auto blockh = std::max<int>(1, h / 180);
    const auto blockw = std::max<int>(1, w / 180);

    // Note: nonsparse. Best performance?
    std::vector<int> count(maiden_count);
    std::vector<floating> snr_sums(maiden_count);

    int last_ts = 0;
    int frame = 0;
    std::vector<std::jthread> threads;
    setaffinity(0);
    ThreadPool pool(get_nprocs());
    setprio(true);
    for (const auto& line : MemLines(data)) {
        // std::cout << line << "\n";
        const auto sp = split<3>(line);
        const auto msg = sp[2];
        const auto okmaiden = get_maiden(msg);
        if (!okmaiden.first) {
            continue;
        }
        const auto snr = parse_int(sp[0]);
        const auto ts = parse_int(sp[1]);
        const auto maiden_index = maidenhead_to_index(okmaiden.second);
        count[maiden_index]++;
        snr_sums[maiden_index] += snr;

        if (last_ts != ts) {
            if (last_ts) {
                pool.add(
                    [frame, snr_sums, &image, count, w, h, blockh, blockw]() mutable {
                        create_frame(frame,
                                     std::move(snr_sums),
                                     std::move(count),
                                     image,
                                     w,
                                     h,
                                     blockh,
                                     blockw);
                    });
                frame++;
            }
            std::ranges::fill(count, 0);
            std::ranges::fill(snr_sums, 0);
            last_ts = ts;
        }
    }
    create_frame(
        frame, std::move(snr_sums), std::move(count), image, w, h, blockh, blockw);
    std::cerr << "Waiting for threads…\n";
}
