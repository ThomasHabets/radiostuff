#include "axlib.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace axlib;

[[noreturn]] void usage(const char* av0, int err)
{
    FILE* f = stdout;
    if (err) {
        f = stderr;
    }
    fprintf(f,
            "Usage: %s […options…] -r <radio> -s <src call> <dst>\n"
            "%s\nExample:\n"
            "   %s -s M0XXX-9 -r radio -p M0ABC-3,M0XYZ-2 "
            "2E0XXX\n",
            av0,
            DGram::common_usage().c_str(),
            av0);
    exit(err);
}

int wrapmain(int argc, char** argv)
{
    CommonOpts copt;
    int opt;
    std::string message = "PING";
    auto lopts = DGram::common_long_opts();
    common_init();
    lopts.push_back({ 0, 0, 0, 0 });
    while ((opt = getopt_long(argc, argv, "ehk:l:m:P:p:r:s:w:", &lopts[0], NULL)) != -1) {
        if (DGram::common_opt(copt, opt)) {
            continue;
        }
        switch (opt) {
        case 'h':
            usage(argv[0], EXIT_SUCCESS);
        case 'm':
            message = optarg;
            break;
        default: /* '?' */
            usage(argv[0], EXIT_FAILURE);
        }
    }

    if (optind + 1 != argc) {
        fprintf(stderr, "Extra args on command line\n");
        exit(EXIT_FAILURE);
    }
    const std::string dst = argv[optind];

    DGram sock(copt.radio, copt.src, copt.path);
    sock.write(dst, message);
    for (;;) {
        const auto packet = sock.recv();
        std::clog << "Got packet src=" << packet.first << " data=<" << packet.second
                  << ">\n";
    }
}
