#include "queue.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <linux/if.h>
#include <linux/if_tun.h>

namespace {
int mainloop(const int sock, const int tunfd, const struct sockaddr_in6* dst)
{
    const int mtu = 1500;
    UDPQueue sockout(sock, mtu, *dst);
    TunQueue tunout(tunfd, mtu, ETH_P_IPV6);
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        FD_SET(tunfd, &rfds);

        fd_set wfds;
        FD_ZERO(&wfds);
        if (!sockout.empty()) {
            FD_SET(sock, &wfds);
        }
        if (!tunout.empty()) {
            FD_SET(tunfd, &wfds);
        }
        const auto rc = select(std::max(sock, tunfd) + 1, &rfds, &wfds, NULL, NULL);

        if (rc < 0) {
            throw std::runtime_error("select()");
        }
        if (rc == 0) {
            // Shouldn't happen.
            continue;
        }

        if (FD_ISSET(tunfd, &rfds)) {
            std::vector<char> buf(mtu);
            const auto rc = read(tunfd, buf.data(), buf.size());
            if (rc > sizeof(struct tun_pi)) {
                buf.resize(rc);
                constexpr auto hlen = sizeof(struct tun_pi);
                sockout.enqueue({ buf.begin() + hlen, buf.end() });
            }
        }

        if (FD_ISSET(sock, &rfds)) {
            std::vector<char> buf(mtu);
            const auto rc = recv(sock, buf.data(), buf.size(), 0);
            if (rc > 0) {
                buf.resize(rc);
                tunout.enqueue(std::move(buf));
            }
        }

        if (FD_ISSET(sock, &wfds)) {
            sockout.send();
        }
        if (FD_ISSET(tunfd, &wfds)) {
            tunout.send();
        }
    }
}

void usage(const char* av0, int err)
{
    printf("Usage: %s -l <port> -t <host:port>\n", av0);
    exit(err);
}
} // namespace

int main(int argc, char** argv)
{
    std::string dev = "radiotun";
    std::string sock_dst_host;
    std::string sock_dst_port;
    int listenport;
    {
        int opt;
        while ((opt = getopt(argc, argv, "d:l:t:"))) {
            switch (opt) {
            case 'd':
                dev = optarg;
                break;
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
                usage(argv[0], EXIT_FAILURE);
            }
        }
    }

    /*
     * Set up UDP.
     */
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock == -1) {
        throw std::runtime_error("failed create socket");
    }

    struct sockaddr_in6 me {};
    me.sin6_family = AF_INET6;
    me.sin6_port = htons(53002);

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
     * Set up tunnel.
     */
    int tunfd;
    if (0 > (tunfd = open("/dev/net/tun", O_RDWR))) {
        throw std::system_error(errno, std::generic_category(), "open(/dev/net/tun)");
    }

    struct ifreq ifr {};
    ifr.ifr_flags = IFF_TUN;

    strncpy(ifr.ifr_name, dev.c_str(), IFNAMSIZ);
    if (0 > ioctl(tunfd, TUNSETIFF, reinterpret_cast<void*>(&ifr))) {
        throw std::system_error(errno, std::generic_category(), "ioctl() for tunnel");
    }

    /*
     * Run.
     */

    return mainloop(sock, tunfd, &peer);
}
