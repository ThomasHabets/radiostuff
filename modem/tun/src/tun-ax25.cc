#include "queue.h"
#include "selectloop.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int mtu = 1500;

int mainloop(const int sock, const int axfd, const struct sockaddr_in6* dst)
{
    UDPQueue sockout(sock, mtu, *dst);
    KISSQueue tunout(axfd, mtu);

    UDPIngress sockin(sock, mtu, tunout);
    KISSIngress tunin(axfd, mtu, sockout);

    return selectloop(sockin, tunin);
}

void usage(const char* av0, int err)
{
    printf("Usage: %s -l <port> -t <host:port>\n", av0);
    exit(err);
}
} // namespace

int main(int argc, char** argv)
{
    std::string sock_dst_host;
    std::string sock_dst_port;
    int listenport;
    {
        int opt;
        while ((opt = getopt(argc, argv, "l:t:")) != -1) {
            switch (opt) {
            case 't': {
                const auto c = strchr(optarg, ':');
                if (c == nullptr) {
                    fprintf(stderr, "Need target to be host:port. Was <%s>\n", optarg);
                    return EXIT_FAILURE;
                }
                sock_dst_host = std::string(optarg, c);
                sock_dst_port = std::string(c + 1);
                break;
            }
            case 'l': {
                char* end = nullptr;
                listenport = strtol(optarg, &end, 0);
                if (*end) {
                    fprintf(stderr, "Need port to be a number. Was <%s>\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            }
            default:
                fprintf(stderr, "Unknown option <%c>\n", opt);
                usage(argv[0], EXIT_FAILURE);
            }
        }
    }

    /*
     * Set up UDP socket.
     */
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        throw std::system_error(errno, std::generic_category(), "socket()");
    }

    struct sockaddr_in6 me {};
    me.sin6_family = AF_INET6;
    me.sin6_port = htons(listenport);

    struct addrinfo hints {};
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_V4MAPPED;

    struct sockaddr_in6 peer {
        .sin6_family = AF_INET6,
    };
    struct addrinfo* res = nullptr;
    if (const int s =
            getaddrinfo(sock_dst_host.c_str(), sock_dst_port.c_str(), &hints, &res)) {
        throw std::runtime_error(std::string("getaddrinfo(): ") + gai_strerror(s));
    }

    for (auto rp = res; rp != nullptr; rp = rp->ai_next) {
        memcpy(&peer, rp->ai_addr, rp->ai_addrlen);
        break;
    }
    freeaddrinfo(res);

    if (-1 == bind(sock, reinterpret_cast<const struct sockaddr*>(&me), sizeof(me))) {
        throw std::system_error(errno, std::generic_category(), "failed to bind()");
    }

    /*
     * Set up PTY.
     */
    int fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "posix_openpt()");
    }
    if (grantpt(fd)) {
        throw std::system_error(errno, std::generic_category(), "grantpt()");
    }
    if (unlockpt(fd)) {
        throw std::system_error(errno, std::generic_category(), "unlockpt()");
    }
    const auto names = ptsname(fd);
    const std::string name = names;

    {
        struct termios ts;
        if (tcgetattr(fd, &ts)) {
            throw std::system_error(errno, std::generic_category(), "tcgetattr()");
        }
        cfmakeraw(&ts);
        ts.c_cc[VMIN] = 1;
        ts.c_cc[VTIME] = 0;
        if (tcsetattr(fd, TCSANOW, &ts)) {
            throw std::system_error(errno, std::generic_category(), "tcsetattr()");
        }
    }

    /*
     * Run.
     */
    std::cerr << "KISS device: " << name << "\n";

    return mainloop(sock, fd, &peer);
}
