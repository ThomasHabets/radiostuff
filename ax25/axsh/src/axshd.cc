#include "axlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

using namespace axlib;

namespace {

void xdup2(int old, int newfd)
{
    if (-1 == dup2(old, newfd)) {
        throw std::system_error(errno, std::generic_category(), "dup2()");
    }
}

class Pipe
{
public:
    Pipe()
    {
        int p[2];
        if (-1 == pipe(p)) {
            throw std::system_error(errno, std::generic_category(), "pipe()");
        }
        r = p[0];
        w = p[1];
    }
    ~Pipe() { close(); }
    void close()
    {
        close_read();
        close_write();
    }
    void close_read()
    {
        if (r != -1) {
            ::close(r);
            r = -1;
        }
    }
    void close_write()
    {

        if (w != -1) {
            ::close(w);
            w = -1;
        }
    }

    void write(const std::string& s)
    {
        const char* p = s.data();
        size_t left = s.size();
        for (;;) {
            const auto rc = ::write(w, p, left);
            if (rc == -1) {
                throw std::system_error(errno, std::generic_category(), "pipe.write()");
            }
            left -= rc;
            p += rc;
            if (left == 0) {
                return;
            }
        }
    }

    // read until eof
    std::string read()
    {
        std::string ret;
        for (;;) {
            char buf[128];
            const auto rc = ::read(r, buf, sizeof(buf));
            if (rc == -1) {
                throw std::system_error(errno, std::generic_category(), "pipe.read()");
            }
            if (rc == 0) {
                return ret;
            }
            ret += std::string(&buf[0], &buf[rc]);
        }
    }

    // No copy.
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;

    int r, w;
};

void shellout(const std::string& cmd, SeqPacket* conn)
{
    Pipe i;
    Pipe o;
    const pid_t pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::generic_category(), "fork()");
    }

    if (pid == 0) {
        xdup2(i.r, 0);
        xdup2(o.w, 1);
        xdup2(o.w, 2);
        i.close();
        o.close();
        execlp("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
        fprintf(stderr, "Failed to exec: %s\n", strerror(errno));
        exit(1);
    }

    i.close_read();
    o.close_write();

    std::thread th([&] {
        try {
            // i.write(in);
            // TODO: allow input
            i.close_write();
        } catch (const std::exception& e) {
            std::clog << "Write to shellout failed: " << e.what();
        }
    });
    try {
        for (;;) {
            auto out = o.read();
            if (out.empty()) {
                break;
            }
            const int step = conn->max_packet_size();
            for (size_t pos = 0; pos < out.size(); pos += step) {
                conn->write(out.substr(pos, step));
            }
        }
    } catch (...) {
        th.join();
        throw;
    }
    th.join();
    int status;
    if (-1 == waitpid(pid, &status, 0)) {
        throw std::system_error(errno, std::generic_category(), "waitpid()");
    }
    if (WIFEXITED(status)) {
        conn->write(">>> Exit status " + std::to_string(WEXITSTATUS(status)) + "\n");
    } else if (WIFSIGNALED(status)) {
        conn->write(">>> killed by signal: " + std::to_string(WTERMSIG(status)) + "\n");
    } else {
        conn->write(">>> died unknown status\n");
    }
}

void handle2(std::unique_ptr<SeqPacket> conn)
{
    for (;;) {
        const auto cmd = conn->read();
        if (cmd.empty()) {
            std::clog << "Client disconnected\n";
            return;
        }
        shellout(cmd, conn.get());
    }
}

void handle(std::unique_ptr<SeqPacket> conn)
{
    try {
        handle2(std::move(conn));
    } catch (const std::exception& e) {
        std::cerr << "Exception in client handler: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in client handler";
        throw;
    }
}

[[noreturn]] void usage(const char* av0, int err)
{
    FILE* f = stdout;
    if (err) {
        f = stderr;
    }
    fprintf(f,
            "Usage: %s […options…] -r <radio> -s <src call>\n"
            "%s\nExample:\n"
            "   %s -r radio6 -k my.priv -P peer.pub -s M0XXX-9\n",
            av0,
            SeqPacket::common_usage().c_str(),
            av0);
    exit(err);
}

} // namespace

int wrapmain(int argc, char** argv)
{
    CommonOpts copt;
    int opt;
    auto lopts = SeqPacket::common_long_opts();
    common_init();
    lopts.push_back({ 0, 0, 0, 0 });
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

    if (optind != argc) {
        fprintf(stderr, "Extra args on command line\n");
        exit(EXIT_FAILURE);
    }

    auto sock = SeqPacket::make_from_commonopts(copt);

    std::clog << "Listening...\n";
    std::vector<std::thread> threads;
    sock->listen([&threads](std::unique_ptr<SeqPacket> conn) {
        std::clog << "Connection from " << conn->peer_addr() << "\n";
        threads.emplace_back(handle, std::move(conn));
    });
    for (auto& th : threads) {
        th.join();
    }
    return 0;
}
