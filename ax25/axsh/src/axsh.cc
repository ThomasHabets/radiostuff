// This Project.
#include "axlib.h"

// External libraries.
#include <histedit.h>

// System.
#include <condition_variable>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>


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

std::pair<std::string, std::string> splitline(const std::string& sv)
{
    const auto i = sv.find('\n');
    if (i == std::string::npos) {
        return { "", std::string(sv) };
    }
    return { sv.substr(0, i + 1), sv.substr(i + 1) };
}

void mainloop(SeqPacket* sock,
              std::mutex& m,
              std::condition_variable& input_cv,
              bool& time_to_die,
              std::string& cmd,
              EditLine* el)
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
    std::string buf;
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
            if (rc == -1) {
                throw std::system_error(errno, std::generic_category(), "select()");
            }
            if (rc == 1) {
                const auto data = sock->read();
                if (data.empty()) {
                    // EOF.
                    std::cout << buf << std::flush;
                    std::cerr << "Server closed connection\n";
                    break;
                }
                buf += data;
                bool printed_anything = false;
                for (;;) {
                    std::string line;
                    std::tie(line, buf) = splitline(buf);
                    if (line.empty()) {
                        break;
                    }
                    struct winsize ws;
                    if (!ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) {
                        std::cout << "\r" << std::string(ws.ws_col - 1, ' ') << "\r";
                    }
                    printed_anything = true;
                    std::cout << line << std::flush;
                }
                if (printed_anything) {
                    el_set(el, EL_REFRESH);
                }
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

std::optional<std::string> get_command(EditLine* el, size_t max)
{
    if (!el) {
        return xgetline(std::cin, max);
    }

    for (;;) {
        int count = 0;
        const char* buf = el_gets(el, &count);
        if (!buf) {
            return {};
        }
        std::string s = buf;
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
            s.pop_back();
        }

        if (!s.empty()) {
            return s;
        }
    }
}

const char* prompt([[maybe_unused]] EditLine* el)
{
    static const char* prompt = "\1axsh>\1 ";
    return prompt;
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

    {
        const auto remaining_args = argc - optind;
        switch (remaining_args) {
        case 0:
            fprintf(stderr, "No destination address provided.\n");
            exit(EXIT_FAILURE);
        case 1:
            // good.
            break;
        default:
            fprintf(stderr, "Too many positional args on command line.\n");
            exit(EXIT_FAILURE);
        }
    }
    const std::string dst = argv[optind];

    EditLine* el = nullptr;
    HistEvent ev;
    History* hist = nullptr;
    if (true) {
        el = el_init(argv[0], stdin, stdout, stderr);
        el_set(el, EL_EDITOR, "emacs");
        el_set(el, EL_PROMPT, prompt, '\1');

        hist = history_init();
        history(hist, &ev, H_SETSIZE, 1000);
        el_set(el, EL_HIST, history, hist);
    }

    auto maybe_sock = SeqPacket::make_from_commonopts(copt);
    if (!maybe_sock.has_value()) {
        std::clog << "Stack: " << maybe_sock.error() << "\n";
        return 1;
    }
    auto sock = std::move(maybe_sock.value());

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
    std::thread user_input([&] {
        for (;;) {
            // Wait for last command to finish.
            {
                std::unique_lock<std::mutex> l(m);
                input_cv.wait(l, [&cmd] { return cmd.empty(); });
            }
            const auto line = get_command(el, sock->max_packet_size()).value_or("exit");
            if (line == "exit") {
                break;
            }
            const bool continuation = false;
            history(hist, &ev, continuation ? H_APPEND : H_ENTER, line.c_str());
            std::unique_lock<std::mutex> l(m);
            cmd = line;
        }
        std::unique_lock<std::mutex> l(m);
        time_to_die = true;
    });
    sleep(1);
    try {
        mainloop(sock.get(), m, input_cv, time_to_die, cmd, el);
    } catch (const std::exception& e) {
        std::cerr << "Exception " << e.what() << "\n";
        user_input.join();
        throw;
    }
    user_input.join();
    return 0;
}
