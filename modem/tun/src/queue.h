// -*- c++ -*-
#ifndef __RADIOSTUFF_TUN_INCLUDE_QUEUE_H__
#define __RADIOSTUFF_TUN_INCLUDE_QUEUE_H__
#include <netinet/in.h>
#include <deque>
#include <vector>

/*
 * Egress queue.
 */
class Queue
{
public:
    Queue(int fd, int mtu) : fd_(fd), mtu_(mtu) {}
    virtual void enqueue(std::vector<char>&& packet);
    bool empty() const noexcept { return queue_.empty(); }
    virtual void send();
    int fd() const { return fd_; }

protected:
    const int fd_;
    const int mtu_;
    std::deque<std::vector<char>> queue_;
};

class TunQueue : public Queue
{
public:
    TunQueue(int fd, int mtu, int proto) : Queue(fd, mtu), proto_(proto) {}
    void enqueue(std::vector<char>&& packet) override;

protected:
    const int proto_;
};

class UDPQueue : public Queue
{
public:
    UDPQueue(int fd, int mtu, struct sockaddr_in6 sa) : Queue(fd, mtu), sa_(sa) {}
    void send() override;

private:
    const struct sockaddr_in6 sa_ {
    };
};

// KISS escape and frame.
class KISSQueue : public Queue
{
public:
    KISSQueue(int fd, int mtu) : Queue(fd, mtu) {}
    void enqueue(std::vector<char>&& packet) override;
};


// Ingress reader.
class Ingress
{
public:
    Ingress(int fd, int mtu, Queue& out) : fd_(fd), mtu_(mtu), out_(out) {}
    virtual bool read();
    int fd() const { return fd_; }
    Queue& out() { return out_; }

protected:
    const int fd_;
    const int mtu_;
    Queue& out_;
};

// Use recv instead of read().
class UDPIngress : public Ingress
{
public:
    UDPIngress(int fd, int mtu, Queue& out) : Ingress(fd, mtu, out) {}
    bool read() override;
};

// Take tun header into account.
class TunIngress : public Ingress
{
public:
    TunIngress(int fd, int mtu, Queue& out) : Ingress(fd, mtu, out) {}
    bool read() override;
};

// Unescape.
class KISSIngress : public Ingress
{
public:
    KISSIngress(int fd, int mtu, Queue& out) : Ingress(fd, mtu, out) {}
    bool read() override;

protected:
    std::vector<char> unparsed_;
};
#endif
