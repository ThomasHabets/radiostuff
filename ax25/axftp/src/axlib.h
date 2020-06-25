// -*- c++ -*-
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

class SeqPacket
{
public:
    SeqPacket(std::string mycall, std::vector<std::string> digipeaters = {});

    SeqPacket(SeqPacket&&) = default;
    virtual ~SeqPacket() = default;
    // SeqPacket& operator=(SeqPacket&&); // Won't work because const mycall_?

    virtual void listen(std::function<void(std::unique_ptr<SeqPacket>)> cb);
    virtual int connect(std::string addr);
    virtual void write(const std::string& msg);
    virtual std::string read();
    std::string peer_addr() const { return peer_addr_; }
    unsigned int window_size() const;
    unsigned int packet_length() const;
    virtual unsigned int max_packet_size() const { return packet_length(); }
    bool extended_modulus() const noexcept { return extended_modulus_; }
    void set_window_size(unsigned int n);
    void set_packet_length(unsigned int n);
    void set_extended_modulus(bool v);

protected:
    SeqPacket(std::string mycall, int sock, std::string peer)
        : sock_(sock), mycall_(std::move(mycall)), peer_addr_(peer)
    {
    }
    void set_parms(int fd);
    void copy_parms(SeqPacket& other) const noexcept;

    Sock sock_;
    const std::string mycall_;
    std::string peer_addr_;
    const std::vector<std::string> digipeaters_;
    unsigned int window_size_ = 0; // use default.
    bool extended_modulus_ = false;
    unsigned int packet_length_ = 0; // default
};


struct CommonOpts {
    std::string src;
    std::vector<std::string> path;
    unsigned int window = 0;
    bool extended_modulus = false;
    int packet_length = 200;
    std::array<char, 32> peer_pub;
    std::array<char, 64> my_priv;
    // TODO: AX25_T1, AX25_T2, AX25_T3, AX25_N2, AX25_BACKOFF,
    // AX25_PIDINCL, AX25_IDLE
};

class SignedSeqPacket : public SeqPacket
{
public:
    SignedSeqPacket(std::string mycall,
                    std::array<char, 64> priv,
                    std::array<char, 32> pub,
                    std::vector<std::string> digipeaters = {})
        : SeqPacket(std::move(mycall), std::move(digipeaters)),
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
    std::array<char, 32> peer_pub_;
    std::array<char, 64> my_priv_;
    std::string nonce_local_;
    std::string nonce_peer_;
    uint64_t packets_sent_{};
    uint64_t packet_good_{}; // good packets received.
    uint64_t packet_bad_{};
    void exchange_nonce();
};

std::unique_ptr<SeqPacket> make_from_commonopts(const CommonOpts& opt);
std::vector<std::string> split(std::string s);

template <int size>
std::array<char, size> load_key(const std::string& fn);
extern template std::array<char, 32> load_key<32>(const std::string& fn);
extern template std::array<char, 64> load_key<64>(const std::string& fn);

unsigned int parse_uint(const std::string& s);

namespace crypto {
bool verify(const std::string& data, const std::array<char, 32>& pk);
std::string sign(const std::string& msg, const std::array<char, 64>& sk);
} // namespace crypto


bool common_opt(CommonOpts& o, int opt);
} // namespace axlib
