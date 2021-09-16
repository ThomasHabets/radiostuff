// -*- c++ -*-
#include <getopt.h>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace axlib {
class Sock
{
public:
    Sock(int s) : fd(s) {}
    ~Sock() { close(); }

    // No copy.
    Sock(const Sock&) = delete;
    Sock& operator=(const Sock&) = delete;

    // Move.
    Sock(Sock&& rhs) noexcept;
    Sock& operator=(Sock&& rhs) noexcept;

    void close() noexcept;
    operator int() const noexcept { return fd; };

private:
    int fd;
};

struct CommonOpts {
    std::string src;
    std::vector<std::string> path;
    std::string radio;
    unsigned int window = 0;
    bool extended_modulus = false;
    int packet_length = 200;
    bool peer_pub_provided = false;
    std::array<char, 32> peer_pub;
    bool my_priv_provided = false;
    std::array<char, 64> my_priv;
    int t1 = -1;
    int t2 = -2;
    int t3 = -1;
    int n2 = -1;
    int backoff = -1;
    int idle = -1;
};


class DGram
{
public:
    DGram(std::string radio,
          std::string mycall,
          std::vector<std::string> digipeaters = {});
    std::pair<std::string, std::string> recv();
    void write(const std::string& dst, const std::string& msg);

    static std::vector<struct option> common_long_opts();
    static bool common_opt(CommonOpts& o, int opt);
    static std::string common_usage();

protected:
    Sock sock_;
    const std::string radio_;
    const std::string mycall_;
    const std::vector<std::string> digipeaters_;
};

class SeqPacket
{
public:
    SeqPacket(std::string radio,
              std::string mycall,
              std::vector<std::string> digipeaters = {});

    SeqPacket(SeqPacket&&) = default;
    virtual ~SeqPacket() = default;
    // SeqPacket& operator=(SeqPacket&&); // Won't work because const mycall_?

    virtual void listen(std::function<void(std::unique_ptr<SeqPacket>)> cb);
    virtual int connect(std::string addr);

    // Write exactly an AX25 frame.
    virtual void write(const std::string& msg);

    // Split the message into max sized frames and call write().
    void write_chunked(const std::string& msg);

    virtual std::string read();
    std::string peer_addr() const { return peer_addr_; }
    unsigned int window_size() const;
    unsigned int packet_length() const;
    virtual unsigned int max_packet_size() const { return packet_length(); }
    bool extended_modulus() const noexcept { return extended_modulus_; }
    void set_window_size(unsigned int n);
    void set_packet_length(unsigned int n);
    void set_extended_modulus(bool v);
    void set_t1(int v);
    void set_t2(int v);
    void set_t3(int v);
    void set_n2(int v);
    void set_backoff(int v);
    void set_idle(int v);
    int get_fd() const noexcept { return sock_; }

    static std::vector<struct option> common_long_opts();
    static bool common_opt(CommonOpts& o, int opt);
    static std::string common_usage();
    static std::unique_ptr<SeqPacket> make_from_commonopts(const CommonOpts& opt);


protected:
    SeqPacket(std::string radio, std::string mycall, int sock, std::string peer)
        : sock_(sock),
          radio_(std::move(radio)),
          mycall_(std::move(mycall)),
          peer_addr_(peer)
    {
    }
    void set_parms(int fd);
    void copy_parms(SeqPacket& other) const noexcept;

    Sock sock_;
    const std::string radio_;
    const std::string mycall_;
    std::string peer_addr_;
    const std::vector<std::string> digipeaters_;
    unsigned int window_size_ = 0; // use default.
    bool extended_modulus_ = false;
    unsigned int packet_length_ = 0; // default

    // Timers.
    int t1_ = -1;
    int t2_ = -2;
    int t3_ = -1;
    int n2_ = -1;
    int backoff_ = -1;
    int idle_ = -1;
};


class SignedSeqPacket : public SeqPacket
{
public:
    SignedSeqPacket(std::string radio,
                    std::string mycall,
                    std::array<char, 64> priv,
                    std::array<char, 32> pub,
                    std::vector<std::string> digipeaters = {})
        : SeqPacket(std::move(radio), std::move(mycall), std::move(digipeaters)),
          my_priv_(priv),
          peer_pub_(pub)
    {
    }
    void listen(std::function<void(std::unique_ptr<SeqPacket>)> cb) override;
    void write(const std::string& msg) override;
    std::string read() override;
    SignedSeqPacket(SeqPacket&&, std::array<char, 64> priv, std::array<char, 32> pub);
    int connect(std::string addr) override;
    unsigned int max_packet_size() const override { return packet_length() - sig_size_; }

private:
    static constexpr int sig_size_ = 64;
    static constexpr int nonce_size_ = 12;
    std::string sign(const std::string&) const;
    std::array<char, 64> my_priv_;
    std::array<char, 32> peer_pub_;
    std::string nonce_local_;
    std::string nonce_peer_;
    uint64_t packets_sent_{};
    uint64_t packet_good_{}; // good packets received.
    uint64_t packet_bad_{};
    void exchange_nonce();
};

std::vector<std::string> split(std::string s, char splitchar);
void common_init();

template <int size>
std::array<char, size> load_key(const std::string& fn);
extern template std::array<char, 32> load_key<32>(const std::string& fn);
extern template std::array<char, 64> load_key<64>(const std::string& fn);

unsigned int parse_uint(const std::string& s);
std::string
xgetline(std::istream& stream, const size_t max, const bool discard_first = false);

namespace crypto {
bool verify(const std::string& data, const std::array<char, 32>& pk);
std::string sign(const std::string& msg, const std::array<char, 64>& sk);
} // namespace crypto


} // namespace axlib
