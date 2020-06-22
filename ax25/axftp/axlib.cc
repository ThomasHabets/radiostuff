#include "axlib.h"
#include <cstring>
#include <iostream>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include <netax25/axlib.h>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace axlib {

namespace {
void set_window_size_fd(int fd, unsigned int window_size) {
  if (window_size > 0) {
    if (-1 == setsockopt(fd, SOL_AX25, AX25_WINDOW, &window_size,
                         sizeof(window_size))) {
      throw std::runtime_error(std::string("setting AX25_WINDOW ") +
                               std::to_string(window_size) + ": " +
                               strerror(errno));
    }
  }
}
void set_extended_modulus_fd(int fd, bool onoff) {
  const unsigned int on = onoff ? 1 : 0;
  if (-1 == setsockopt(fd, SOL_AX25, AX25_EXTSEQ, &on, sizeof(on))) {
    throw std::runtime_error(std::string("setting AX25_EXTSEQ ") +
                             std::to_string(on) + ": " + strerror(errno));
  }
}
void set_packet_length_fd(int fd, unsigned int len) {
  if (len == 0) {
    return;
  }
  if (-1 == setsockopt(fd, SOL_AX25, AX25_PACLEN, &len, sizeof(len))) {
    throw std::runtime_error(std::string("setting AX25_PACLEN ") +
                             std::to_string(len) + ": " + strerror(errno));
  }
}

} // namespace

unsigned int parse_uint(const std::string &s) {
  const char *p = s.data();
  char *ep;
  auto ret = strtoul(p, &ep, 0);
  if (*ep != 0) {
    throw std::runtime_error(s + " is not a number");
  }
  return ret;
}

bool common_opt(CommonOpts &o, int opt) {
  switch (opt) {
  case 'e':
    o.extended_modulus = true;
    break;
  case 'p':
    o.path = split(optarg);
    break;
  case 's':
    o.src = optarg;
    break;
  case 'w':
    o.window = parse_uint(optarg);
    break;
  case 'l':
    o.packet_length = parse_uint(optarg);
    break;
  default:
    return false;
  }
  return true;
}

std::vector<std::string> split(std::string s) {
  std::stringstream ss{s};
  std::string word;
  std::vector<std::string> ret;
  while (std::getline(ss, word, ',')) {
    ret.push_back(word);
  }
  return ret;
}

SeqPacket::SeqPacket(std::string mycall, std::vector<std::string> digipeaters)
    : sock_(socket(AF_AX25, SOCK_SEQPACKET, 0)), mycall_(std::move(mycall)),
      digipeaters_(std::move(digipeaters)) {
  if (sock_ == -1) {
    throw std::runtime_error(
        std::string("socket(AF_AX25, SOCK_SEQPACKET, 0): ") + strerror(errno));
  }
  set_parms(sock_);

  struct full_sockaddr_ax25 me {};
  if (-1 == ax25_aton(mycall_.c_str(), &me)) {
    throw std::runtime_error(std::string("ax25_aton") + strerror(errno));
  }
  me.fsa_ax25.sax25_ndigis = std::min(
      digipeaters_.size(), decltype(digipeaters_)::size_type(AX25_MAX_DIGIS));
  for (int i = 0; i < me.fsa_ax25.sax25_ndigis; i++) {
    ax25_aton_entry(digipeaters_[i].c_str(),
                    reinterpret_cast<char *>(&me.fsa_digipeater[i]));
  }

  if (-1 == bind(sock_, reinterpret_cast<struct sockaddr *>(&me), sizeof(me))) {
    throw std::runtime_error("bind to " + mycall_ +
                             " failed: " + strerror(errno));
  }
}

int SeqPacket::connect(std::string addr) {
  peer_addr_ = std::move(addr);
  struct sockaddr_ax25 peer {};
  peer.sax25_family = AF_AX25;
  peer.sax25_ndigis = 0; // TODO?
  ax25_aton_entry(peer_addr_.c_str(),
                  reinterpret_cast<char *>(&peer.sax25_call));

  if (-1 == ::connect(sock_, reinterpret_cast<struct sockaddr *>(&peer),
                      sizeof(peer))) {
    return errno;
  }
  if (false) {
    const int on = 1;
    if (-1 == setsockopt(sock_, SOL_AX25, AX25_EXTSEQ, &on, sizeof(on))) {
      throw std::runtime_error(std::string("setting AX25_EXTSEQ ") +
                               strerror(errno));
    }
  }
  set_parms(sock_);
  return 0;
}

void SeqPacket::write(const std::string &msg) {
  ssize_t rc;
  do {
    rc = ::write(sock_, msg.data(), msg.size());
  } while (rc == -1 && errno == EINTR);
  if (rc != msg.size()) {
    throw std::runtime_error(std::string("write(): ") + strerror(errno));
  }
}

std::string SeqPacket::read() {
  std::array<char, 1000> buf;
  ssize_t rc;
  do {
    rc = ::read(sock_, buf.data(), buf.size());
  } while (rc == -1 && errno == EINTR);
  if (rc == -1) {
    throw std::runtime_error(std::string("read(): ") + strerror(errno));
  }
  return std::string(&buf[0], &buf[rc]);
}

void SeqPacket::listen(std::function<void(SeqPacket)> cb) {
  if (-1 == ::listen(sock_, 5)) {
    throw std::runtime_error(std::string("listen(): ") + strerror(errno));
  }

  for (;;) {
    struct full_sockaddr_ax25 peer {};
    socklen_t len = sizeof(peer);
    int fd = accept(sock_, reinterpret_cast<struct sockaddr *>(&peer), &len);
    if (fd == -1) {
      fprintf(stderr, "accept(): %s\n", strerror(errno));
      continue;
    }
    set_parms(fd);
    auto p = reinterpret_cast<const ax25_address *>(&peer.fsa_ax25.sax25_call);
    SeqPacket s{mycall_, fd, ax25_ntoa(p)};
    copy_parms(s);
    cb(std::move(s));
  }
}

void SeqPacket::set_parms(int fd) {
  set_window_size_fd(fd, window_size_);
  set_packet_length_fd(fd, packet_length_);
  set_extended_modulus_fd(fd, extended_modulus_);
}

void SeqPacket::copy_parms(SeqPacket &other) const noexcept {
  other.set_window_size(window_size_);
  other.set_extended_modulus(extended_modulus_);
  other.set_packet_length(packet_length_);
}

unsigned int SeqPacket::window_size() const {
  unsigned int ret;
  socklen_t len = sizeof(ret);
  if (-1 == getsockopt(sock_, SOL_AX25, AX25_WINDOW, &ret, &len)) {
    throw std::runtime_error(std::string("getting AX25_WINDOW: ") +
                             strerror(errno));
  }
  return ret;
}

unsigned int SeqPacket::packet_length() const {
  unsigned int ret;
  socklen_t len = sizeof(ret);
  if (-1 == getsockopt(sock_, SOL_AX25, AX25_PACLEN, &ret, &len)) {
    throw std::runtime_error(std::string("getting AX25_PACLEN: ") +
                             strerror(errno));
  }
  return ret;
}

void Sock::close() noexcept {
  if (fd != -1) {
    ::close(fd);
    fd = -1;
  }
}

Sock::Sock(Sock &&rhs) noexcept { *this = std::move(rhs); }
Sock &Sock::operator=(Sock &&rhs) noexcept {
  fd = rhs.fd;
  rhs.fd = -1;
  return *this;
}
} // namespace axlib
