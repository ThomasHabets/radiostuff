#include "queue.h"
#include "selectloop.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <linux/if.h>
#include <linux/if_tun.h>

namespace {
int mtu = 1500;

int mainloop(const int sock, const int tunfd, const struct sockaddr_in6* dst)
{
    UDPQueue sockout(sock, mtu, *dst);
    TunQueue tunout(tunfd, mtu, ETH_P_IPV6);

    UDPIngress sockin(sock, mtu, tunout);
    TunIngress tunin(tunfd, mtu, sockout);
    return selectloop(sockin, tunin);
}

void usage(const char* av0, int err)
{
    printf("Usage: %s -l <port> -t <host:port>\n"
           "  -l <port>        UDP port to receive on.\n"
           "  -t <host:port>   UDP address to send packets to.\n"
           "  -d <name>        Name of device.\n"
           "  -h               Show this help text\n"
           "  -m <mtu>         MTU.\n",
           av0);
    exit(err);
}
} // namespace

int main(int argc, char** argv)
{
    std::string dev = "radiotun";
    std::string sock_dst_host;
    std::string sock_dst_port;
    int listenport = -1;
    {
        int opt;
        while ((opt = getopt(argc, argv, "hd:l:t:m:")) != -1) {
            switch (opt) {
            case 'h':
                usage(argv[0], EXIT_SUCCESS);
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
            case 'm': {
                char* end = nullptr;
                mtu = strtol(optarg, &end, 0);
                if (*end || mtu < 1) {
                    fprintf(stderr, "MTU must be >0. Was <%s>\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            }
            default:
                usage(argv[0], EXIT_FAILURE);
            }
        }
    }

    if (listenport < 0) {
        fprintf(stderr, "-l <port> is mandatory\n");
        return EXIT_FAILURE;
    }

    if (argc != optind) {
        fprintf(stderr, "Trailing args on the command line\n");
        return EXIT_FAILURE;
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

    ifr.ifr_mtu = mtu;
    if (ioctl(sock, SIOCSIFMTU, reinterpret_cast<caddr_t*>(&ifr)) < 0) {
        throw std::system_error(errno, std::generic_category(), "ioctl() to set MTU");
    }

    /*
     * Run.
     */

    return mainloop(sock, tunfd, &peer);
}
