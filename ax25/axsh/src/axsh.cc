#include <condition_variable>
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
            "2E0XXX-9\n",
            av0,
            SeqPacket::common_usage().c_str(),
            av0);
    exit(err);
}

void mainloop(SeqPacket* sock,
              std::mutex& m,
              std::condition_variable& input_cv,
              bool& time_to_die,
              std::string& cmd)
{
    // We have to use select() instead of a thread. A thread would be
    // nicer (no polling needed wakeups), but the Linux kernel doesn't
    // support threads calling write() and read() at the same time on
    // the same AX_25 socket. I've sent a patch:
    // https://marc.info/?l=linux-hams&m=159319049624305&w=2
    //
    // Yes, I could make this 100% event-based, but it's just so nice
    // to not have to deal with partial command buffers that I
    // preferred this.
    for (;;) {
        // Check if done.
        {
            std::unique_lock<std::mutex> l(m);
            if (time_to_die) {
                break;
            }
        }

        // Check if any data is received.
        {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock->get_fd(), &fds);
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            const auto rc = select(sock->get_fd() + 1, &fds, NULL, NULL, &tv);
            if (rc == 1) {
                const auto data = sock->read();
                if (data.empty()) {
                    // EOF.
                    std::cerr << "Server closed connection\n";
                    break;
                }
                std::cout << data << std::flush;
            } else if (rc == -1) {
                throw std::runtime_error(std::string("select(): ") + strerror(errno));
            }
        }

        // Write anything queued up to write.
        {
            std::unique_lock<std::mutex> l(m);
            if (!cmd.empty()) {
                std::clog << "]]] Sending command <" << cmd << ">" << std::endl;
                sock->write(cmd);
                cmd = "";
                input_cv.notify_one();
            }
        }
    }
}
} // namespace

int wrapmain(int argc, char** argv)
{
    CommonOpts copt;
    int opt;
    auto lopts = SeqPacket::common_long_opts();
    lopts.push_back({ 0, 0, 0, 0 });
    common_init();
    while ((opt = getopt_long(argc, argv, "ehk:l:P:p:r:s:w:", &lopts[0], NULL)) != -1) {
        if (SeqPacket::common_opt(copt, opt)) {
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
        fprintf(stderr, "Need dest\n");
        exit(EXIT_FAILURE);
    }
    const std::string dst = argv[optind];

    auto sock = SeqPacket::make_from_commonopts(copt);

    std::clog << "Connecting...\n";
    const auto rc = sock->connect(dst);
    if (rc) {
        std::clog << "Failed to connect: " << strerror(rc) << std::endl;
        return 1;
    }
    std::clog << "Connected!\n";

    std::mutex m;
    std::string cmd;
    bool time_to_die = false;
    std::condition_variable input_cv;
    std::thread reader([&] {
        for (;;) {
            // Wait for last command to finish.
            {
                std::unique_lock<std::mutex> l(m);
                input_cv.wait(l, [&cmd] { return cmd.empty(); });
            }
            const auto line = xgetline(std::cin, sock->max_packet_size());

            if (!std::cin.good() || line == "exit") {
                break;
            }
            if (!std::cin.good()) {
                throw std::runtime_error(std::string("stdin read failure: ") +
                                         strerror(errno));
            }
            std::unique_lock<std::mutex> l(m);
            cmd = line;
        }
        std::unique_lock<std::mutex> l(m);
        time_to_die = true;
    });
    sleep(1);
    try {
        mainloop(sock.get(), m, input_cv, time_to_die, cmd);
    } catch (const std::exception& e) {
        std::cerr << "Exception " << e.what() << "\n";
        reader.join();
        throw;
    }
    reader.join();
    return 0;
}
