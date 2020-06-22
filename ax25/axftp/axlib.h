// -*- c++ -*-
#include <functional>
#include <string>
#include <vector>

namespace axlib {
class Sock {
public:
  Sock(int s) : fd(s) {}
  ~Sock() { close(); }

  // No copy.
  Sock(const Sock &) = delete;
  Sock &operator=(const Sock &) = delete;

  // Move.
  Sock(Sock &&rhs) noexcept;
  Sock &operator=(Sock &&rhs) noexcept;

  void close() noexcept;
  operator int() const noexcept { return fd; };

private:
  int fd;
};

class SeqPacket {
public:
  SeqPacket(std::string mycall, std::vector<std::string> digipeaters = {});

  SeqPacket(SeqPacket &&) = default;
  // SeqPacket& operator=(SeqPacket&&); // Won't work because const mycall_?

  void listen(std::function<void(SeqPacket)> cb);
  int connect(std::string addr);
  void write(const std::string &msg);
  std::string read();
  std::string peer_addr() const { return peer_addr_; }
  unsigned int window_size() const;
  unsigned int packet_length() const;
  void set_window_size(unsigned int n) noexcept { window_size_ = n; }
  void set_packet_length(unsigned int n) noexcept { packet_length_ = n; }
  bool extended_modulus() const noexcept { return extended_modulus_; }
  void set_extended_modulus(bool v) noexcept { extended_modulus_ = v; }

private:
  SeqPacket(std::string mycall, int sock, std::string peer)
      : sock_(sock), mycall_(std::move(mycall)), peer_addr_(peer) {}
  void set_parms(int fd);
  void copy_parms(SeqPacket &other) const noexcept;

  Sock sock_;
  const std::string mycall_;
  std::string peer_addr_;
  const std::vector<std::string> digipeaters_;
  unsigned int window_size_ = 0; // use default.
  bool extended_modulus_ = false;
  unsigned int packet_length_ = 0; // default
};

std::vector<std::string> split(std::string s);
struct CommonOpts {
  std::string src;
  std::vector<std::string> path;
  unsigned int window = 0;
  bool extended_modulus = false;
  int packet_length = 200;
  // TODO: AX25_T1, AX25_T2, AX25_T3, AX25_N2, AX25_BACKOFF,
  // AX25_PIDINCL, AX25_IDLE
};

unsigned int parse_uint(const std::string &s);

bool common_opt(CommonOpts &o, int opt);
} // namespace axlib
