#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>


#include "axlib.h"

using namespace axlib;

namespace {
void usage(const char* av0, int err)
{
    FILE* f = stdout;
    if (err) {
        f = stderr;
    }
    fprintf(f,
            "Usage: %s […options…] -s <src call> <dst>\n"
            "%s\nExample:\n"
            "   %s -k my.priv -P peer.pub -s M0XXX-9 -p M0XXX-0 2E0XXX-9\n",
            av0,
            common_usage().c_str(),
            av0);
    exit(err);
}
} // namespace

int main(int argc, char** argv)
{
    CommonOpts copt;
    int opt;
    auto lopts = common_long_opts();
    lopts.push_back({ 0, 0, 0, 0 });
    while ((opt = getopt_long(argc, argv, "ehk:l:P:p:s:w:", &lopts[0], NULL)) != -1) {
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

    std::mutex m;
    std::string cmd;
    std::thread reader([&] {
        for (;;) {
            {
                std::unique_lock<std::mutex> l(m);
                if (!cmd.empty()) {
                    continue;
                }
            }

            std::cout << "prompt\n";
            std::vector<char> buf(128);
            std::cin.getline(&buf[0], sizeof(buf));
            const auto len = std::cin.gcount();
            const auto line = std::string(&buf[0], &buf[len - 1]);
            std::clog << "writing\n";

            std::unique_lock<std::mutex> l(m);
            cmd = line;
        }
    });
    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock->get_fd(), &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        const auto rc = select(sock->get_fd() + 1, &fds, NULL, NULL, &tv);
        {
            std::unique_lock<std::mutex> l(m);
            if (!cmd.empty()) {
                sock->write(cmd);
                cmd = "";
            }
        }
        if (rc == 1) {
            std::cout << sock->read() << std::flush;
        }
    }
    reader.join();
    return 0;
}
