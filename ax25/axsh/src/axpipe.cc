#include "axlib.h"

#include <sys/select.h>
#include <unistd.h>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <optional>

using namespace axlib;

namespace {

size_t stdin_chunk_size = 120;

[[noreturn]] void usage(const char* av0, int err)
{
    FILE* f = stdout;
    if (err) {
        f = stderr;
    }
    fprintf(
        f,
        "Usage: %s […options…] -r <radio> -s <src call> [ -c <dst> ]\n"
        "%s\n"
        "    -C <chunk size>        Amount to read from stdin and send as seqpacket.\n"
        "Example:\n"
        "   %s -k my.priv -P peer.pub -s M0XXX-9 -r radio -p M0ABC-3,M0XYZ-2 "
        "-c 2E0XXX-9\n",
        av0,
        SeqPacket::common_usage().c_str(),
        av0);
    exit(err);
}

template <typename T>
std::optional<T> parse_int(std::string_view sv)
{
    T i;
    const auto [ptr, ec] = std::from_chars(sv.begin(), sv.end(), i);
    if (ec != std::errc()) {
        return {};
    }
    if (ptr != sv.end()) {
        return {};
    }
    return i;
}

void handle(SeqPacket& conn)
{
    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(conn.get_fd(), &fds);
        FD_SET(STDIN_FILENO, &fds);
        const auto rc = select(
            std::max(conn.get_fd(), STDIN_FILENO) + 1, &fds, nullptr, nullptr, nullptr);
        if (rc == -1) {
            throw std::system_error(errno, std::generic_category(), "select()");
        }
        if (FD_ISSET(conn.get_fd(), &fds)) {
            const auto data = conn.read();
            if (data.empty()) {
                std::cerr << "Remote end closed connection\n";
                return;
            }
            for (std::string_view part = data; !part.empty();) {
                const auto rc = write(STDOUT_FILENO, part.data(), part.size());
                if (rc == -1) {
                    throw std::system_error(
                        errno, std::generic_category(), "write(STDOUT_FILENO)");
                }
                part = part.substr(rc);
            }
        }
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            std::vector<char> buf(stdin_chunk_size);
            const auto rc = read(STDIN_FILENO, buf.data(), buf.size());
            if (rc == -1) {
                throw std::system_error(
                    errno, std::generic_category(), "read(STDIN_FILENO)");
            }
            const std::string data(&buf[0], &buf[rc]);
            if (data.empty()) {
                std::cerr << "STDIN closed\n";
                return;
            }
            conn.write(data);
        }
    }
}
} // namespace

int wrapmain(int argc, char** argv)
{
    CommonOpts copt;
    common_init();

    std::string dst = "";

    auto lopts = SeqPacket::common_long_opts();
    lopts.push_back({ 0, 0, 0, 0 }); // End of list.
    {
        int opt;
        while ((opt = getopt_long(argc, argv, "c:C:ehk:l:P:p:r:s:w:", &lopts[0], NULL)) !=
               -1) {
            if (SeqPacket::common_opt(copt, opt)) {
                continue;
            }
            switch (opt) {
            case 'c':
                dst = optarg;
                break;
            case 'C':
                if (auto v = parse_int<size_t>(optarg); !v) {
                    std::cerr << "Chunk size given is not a number: " << optarg << "\n";
                    exit(EXIT_FAILURE);
                } else {
                    stdin_chunk_size = v.value();
                }
                break;
            case 'h':
                usage(argv[0], EXIT_SUCCESS);
            default: /* '?' */
                usage(argv[0], EXIT_FAILURE);
            }
        }
    }

    if (optind != argc) {
        std::cerr << "Trailing args on command line\n";
        exit(EXIT_FAILURE);
    }
    auto sock = SeqPacket::make_from_commonopts(copt);
    if (dst.empty()) {
        std::clog << "Listening…\n";
        sock->listen([](std::unique_ptr<SeqPacket> conn) {
            std::clog << "Connection from " << conn->peer_addr() << "\n";
            handle(*conn);
            exit(0);
        });
    } else {
        std::clog << "Connecting…\n";
        const auto rc = sock->connect(dst);
        if (rc) {
            std::clog << "Failed to connect: " << strerror(rc) << std::endl;
            return 1;
        }
        std::clog << "Connected!\n";
        handle(*sock);
    }
    return 0;
}
