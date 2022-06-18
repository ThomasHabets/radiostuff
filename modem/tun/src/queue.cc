#include "queue.h"

#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <linux/if_tun.h>

void Queue::enqueue(std::vector<char>&& buf) { queue_.push_back(std::move(buf)); }


// TODO: log errors
void Queue::send()
{
    auto buf = queue_.front();
    const auto rc = write(fd_, buf.data(), buf.size());
    if (static_cast<size_t>(rc) != buf.size()) {
        fprintf(stderr, "write()=%zd: %s", rc, strerror(errno));
        return;
    }
    queue_.pop_front();
}

void TunQueue::enqueue(std::vector<char>&& buf)
{
    constexpr auto hlen = sizeof(struct tun_pi);
    buf.resize(buf.size() + hlen);

    std::rotate(buf.rbegin(), buf.rbegin() + hlen, buf.rend());
    const auto pi = reinterpret_cast<struct tun_pi*>(buf.data());
    pi->flags = 0;
    pi->proto = htons(proto_);
    queue_.push_back(std::move(buf));
}

// TODO: log errors
void UDPQueue::send()
{
    auto buf = queue_.front();
    if (-1 == sendto(fd_,
                     buf.data(),
                     buf.size(),
                     0,
                     reinterpret_cast<const struct sockaddr*>(&sa_),
                     sizeof(sa_))) {
        perror("sendto()");
        return;
    }
    fprintf(stderr, "UDPQueue> sent %zd bytes\n", buf.size());
    queue_.pop_front();
}

constexpr auto FEND = static_cast<char>(0xC0);
constexpr auto FESC = static_cast<char>(0xDB);
constexpr auto TFEND = static_cast<char>(0xDC);
constexpr auto TFESC = static_cast<char>(0xDD);

void KISSQueue::enqueue(std::vector<char>&& buf)
{
    std::vector<char> t;
    t.reserve(buf.size() * 2);
    t.push_back(FEND);
    t.push_back(0);
    printf("KISS Enqueue> ");
    for (const auto ch : buf) {
        printf("%02x ", (uint8_t)ch);
        switch (ch) {
        case FESC:
            t.push_back(FESC);
            t.push_back(TFESC);
            break;
        case FEND:
            t.push_back(FESC);
            t.push_back(TFEND);
            break;
        default:
            t.push_back(ch);
        }
    }
    t.push_back(FEND);
    printf("\n");
    queue_.push_back(std::move(t));
}

bool Ingress::read()
{
    std::vector<char> buf(mtu_);
    const auto rc = ::read(fd_, buf.data(), buf.size());
    if (rc <= 0) {
        std::cerr << "Failed to read: " << strerror(errno) << "\n";
        return false;
    }
    buf.resize(rc);
    out_.enqueue(std::move(buf));
    return true;
}

bool UDPIngress::read()
{
    std::vector<char> buf(mtu_);
    const auto rc = recv(fd_, buf.data(), buf.size(), 0);
    if (rc <= 0) {
        std::cerr << "UDPIngress: failed to read: " << strerror(errno) << "\n";
        return false;
    }
    buf.resize(rc);
    out_.enqueue(std::move(buf));
    return true;
}

bool TunIngress::read()
{
    constexpr ssize_t hlen = sizeof(struct tun_pi);
    std::vector<char> buf(mtu_ + hlen);
    const auto rc = ::read(fd_, buf.data(), buf.size());
    if (rc <= hlen) {
        std::cerr << "TunIngress: failed to read: " << strerror(errno) << "\n";
        return false;
    }
    buf.resize(rc);
    // TODO: Sanity check the header.
    out_.enqueue({ buf.begin() + hlen, buf.end() });
    return true;
}

bool KISSIngress::read()
{
    {
        std::vector<char> buf(mtu_);
        const auto rc = ::read(fd_, buf.data(), buf.size());
        if (rc <= 0) {
            std::cerr << "KISSIngress: failed to read: " << strerror(errno) << "\n";
            return false;
        }
        buf.resize(rc);
        unparsed_.insert(unparsed_.end(), buf.begin(), buf.end());
    }
    bool fail = false;

    std::vector<char> packet;
    for (size_t i = 0; i < unparsed_.size(); i++) {
        if (fail) {
            if (unparsed_[i] == FEND) {
                fail = false;
                unparsed_.erase(unparsed_.begin(), unparsed_.begin() + i + 1);
                i = 0;
            }
            continue;
        }

        if (unparsed_[i] == FESC) {
            i++;
            if (i == unparsed_.size()) {
                // Packet in the middle of an escape.
                return true;
            }
            switch (unparsed_[i]) {
            case TFEND:
                packet.push_back(FEND);
                break;
            case TFESC:
                packet.push_back(FESC);
                break;
            default:
                // TODO: bad state, wait for FEND.
                fail = true;
                packet.clear();
                break;
            }
            continue;
        }

        if (unparsed_[i] == FEND) {
            if (!packet.empty()) {
                out_.enqueue(std::move(packet));
            }
            unparsed_.erase(unparsed_.begin(), unparsed_.begin() + i + 1);
            i = 0;
            continue;
        }
        packet.push_back(unparsed_[i]);
    }
    return true;
}
