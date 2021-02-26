#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

#include "axlib.h"

using namespace axlib;

namespace {
[[noreturn]] void usage(const char* av0, int err)
{
    FILE* f = stdout;
    if (err) {
        f = stderr;
    }
    fprintf(f,
            "Usage: %s […options…] -r <radio> -s <src call> <dst>\n"
            "%s\nExample:\n"
            "   %s -k my.priv -P peer.pub -s M0XXX-9 -r radio -p M0ABC-3,M0XYZ-2 "
            "2E0XXX\n",
            av0,
            common_usage().c_str(),
            av0);
    exit(err);
}
} // namespace

int wrapmain(int argc, char** argv)
{
    CommonOpts copt;
    int opt;
    auto lopts = common_long_opts();
    common_init();
    lopts.push_back({ 0, 0, 0, 0 });
    while ((opt = getopt_long(argc, argv, "ehk:l:P:p:r:s:w:", &lopts[0], NULL)) != -1) {
        if (common_opt(copt, opt)) {
            continue;
        }
        switch (opt) {
        case 'h':
            usage(argv[0], EXIT_SUCCESS);
        default: /* '?' */
            usage(argv[0], EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Need dest\n");
        exit(EXIT_FAILURE);
    }
    const std::string dst = argv[optind];

    auto sock = make_from_commonopts(copt);

    std::clog << "Connecting...\n";
    if (sock->connect(dst)) {
        std::clog << "Failed to connect!\n";
        return 1;
    }
    std::clog << "Connected!\n";
    for (;;) {
        const auto cmd = axlib::xgetline(std::cin, 1000);
        if (cmd == "") {
            // EOF.
            break;
        }
        std::clog << "Sending command <" << cmd << ">\n";
        sock->write(cmd + "\n");

        const auto resp = sock->read();
        char* end = nullptr;
        const auto total = strtoul(resp.data(), &end, 0);
        if (*end != 0) {
            std::cerr << "> " << resp << std::endl;
            continue;
        }

        std::clog << "Total size: " << total << std::endl;
        auto rcvd = decltype(total){ 0 };
        for (;;) {
            const auto s = sock->read();
            if (s.empty()) {
                break;
            }
            rcvd += s.size();
            // TODO: progress bar.
            std::cerr << "Got " << s.size() << " total: " << rcvd << std::endl;
            std::cout << s << std::flush;
            if (total == rcvd) {
                break;
            }
        }
    }
    return 0;
}
