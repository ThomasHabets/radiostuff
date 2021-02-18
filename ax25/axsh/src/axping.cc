#include "axlib.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace axlib;

[[noreturn]] void usage(const char* av0, int err)
{
    fprintf(stderr, "%s: Usage\n", av0);
    exit(err);
}

int main(int argc, char** argv)
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

    if (optind + 1 != argc) {
        fprintf(stderr, "Extra args on command line\n");
        exit(EXIT_FAILURE);
    }
    const std::string dst = argv[optind];

    DGram sock(copt.radio, copt.src, copt.path);
    sock.write(dst, "hello");
    for (;;) {
        const auto packet = sock.recv();
        std::clog << "Got packet src=" << packet.first << " data=<" << packet.second
                  << ">\n";
    }
}
