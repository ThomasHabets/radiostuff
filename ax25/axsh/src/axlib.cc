#include "axlib.h"
#include <getopt.h>
#include <netax25/ax25.h>
#include <netax25/axconfig.h>
#include <netax25/axlib.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace axlib {

namespace {
enum class coptval {
    t1 = 1,
    t2 = 2,
    t3 = 3,
    n2 = 4,
    backoff = 5,
    idle = 6,
};

void set_window_size_fd(int fd, unsigned int window_size)
{
    if (window_size > 0) {
        if (-1 ==
            setsockopt(fd, SOL_AX25, AX25_WINDOW, &window_size, sizeof(window_size))) {
            throw std::system_error(errno,
                                    std::generic_category(),
                                    "setting AX25_WINDOW to " +
                                        std::to_string(window_size));
        }
    }
}

void set_int_fd(int fd, int opt, int val)
{
    if (val > 0) {
        if (-1 == setsockopt(fd, SOL_AX25, opt, &val, sizeof(val))) {
            throw std::system_error(errno,
                                    std::generic_category(),
                                    "setting opt " + std::to_string(opt) + " to " +
                                        std::to_string(val));
        }
    }
}

void set_extended_modulus_fd(int fd, bool onoff)
{
    const unsigned int on = onoff ? 1 : 0;
    if (-1 == setsockopt(fd, SOL_AX25, AX25_EXTSEQ, &on, sizeof(on))) {
        throw std::system_error(errno,
                                std::generic_category(),
                                "setting AX25_EXTSEQ to " + std::to_string(on));
    }
}
void set_packet_length_fd(int fd, unsigned int len)
{
    if (len == 0) {
        return;
    }
    if (-1 == setsockopt(fd, SOL_AX25, AX25_PACLEN, &len, sizeof(len))) {
        throw std::system_error(errno,
                                std::generic_category(),
                                "setting AX25_PACLEN to " + std::to_string(len));
    }
}

} // namespace

void common_init()
{
    const auto ports = ax25_config_load_ports();
    if (!ports) {
        std::cerr << "Warning: zero AX25 ports available. This tool will likely not be "
                     "able to run.\n";
    }
}

std::unique_ptr<SeqPacket> SeqPacket::make_from_commonopts(const CommonOpts& copt)
{
    if (copt.my_priv_provided != copt.peer_pub_provided) {
        throw std::runtime_error(
            "if priv key is provided, pubkey must be too. And vice versa");
    }
    std::unique_ptr<SeqPacket> sock;
    if (copt.my_priv_provided) {
        sock = std::make_unique<SignedSeqPacket>(
            copt.radio, copt.src, copt.my_priv, copt.peer_pub, copt.path);
    } else {
        sock = std::make_unique<SeqPacket>(copt.radio, copt.src, copt.path);
    }
    sock->set_extended_modulus(copt.extended_modulus);
    sock->set_window_size(copt.window);
    sock->set_packet_length(copt.packet_length);
    sock->set_t1(copt.t1);
    sock->set_t2(copt.t2);
    sock->set_t3(copt.t3);
    sock->set_n2(copt.n2);
    sock->set_backoff(copt.backoff);
    sock->set_idle(copt.idle);
    return sock;
}

std::string xgetline(std::istream& stream, const size_t max, const bool discard_first)
{
    std::vector<char> buf(max + 1); // Since getline fails at count-1 bytes.
    std::cin.getline(&buf[0], buf.size());

    if (std::cin.eof()) {
        return "";
    }

    // Failbit is set if count-1 bytes have been read.
    if (std::cin.fail()) {
        if (!discard_first) {
            std::clog << "]]] Command too long. Discarding.\n";
        }
        std::cin.clear();
        return xgetline(stream, max, true);
    }

    // Got complete line, but it could be a tail end of a too large line.
    if (discard_first) {
        return xgetline(stream, max);
    }

    const auto len = std::cin.gcount();
    if (len == 0) {
        return "";
    }

    return std::string(&buf[0], &buf[len - 1]);
}

unsigned int parse_uint(const std::string& s)
{
    const char* p = s.data();
    char* ep;
    auto ret = strtoul(p, &ep, 0);
    if (*ep != 0) {
        throw std::runtime_error(s + " is not a number");
    }
    return ret;
}

std::string DGram::common_usage()
{
    return "    -p, --path=<path>      AX25 path to route through. Comma "
           "separated.\n"
           "    -r, --radio=<radio>    Radio port name per /etc/ax25/axports.\n"
           "    -s, --mycall=<num>     Local callsign\n";
}

std::string SeqPacket::common_usage()
{
    return DGram::common_usage() +
           "    --t1=<num>             Seconds to wait before retransmitting an\n"
           "                           unacknowledged frame.\n"
           "    --t2=<num>             Minimum seconds to wait for another frame "
           "to be\n"
           "                           received before transmitting an "
           "acknowledgement.\n"
           "    --t3=<num>             Seconds to wait between sending a check "
           "that the\n"
           "                           link is still active.\n"
           "    --n2=<num>             How many times to retransmit a frame "
           "before assuming\n"
           "                           the connection has failed.\n"
           "    --backoff=<num>        Set 0 for linear backoff, 1 for "
           "exponential backoff.\n"
           "    --idle=<num>           The period of time a connection can be "
           "idle before\n"
           "                           we close it down.\n"
           "    -P, --peer_pub=<file>  Peer public key.\n"
           "    -k, --my_priv=<file>   Local private key.\n"
           "                           frames.\n"
           "    -l, --paclen=<num>     Max packet length\n"
           "    -w, --window=<num>     The maximum number of unacknowledged "
           "transmitted\n"
           "    -e, --extseq           Enable extended sequence numbers. Needed "
           "for large\n"
           "                           windows.\n";
}

std::vector<struct option> SeqPacket::common_long_opts()
{
    std::vector<struct ::option> ret{
        { "t1", required_argument, 0, int(coptval::t1) },
        { "t2", required_argument, 0, int(coptval::t2) },
        { "t3", required_argument, 0, int(coptval::t3) },
        { "n2", required_argument, 0, int(coptval::n2) },
        { "backoff", required_argument, 0, int(coptval::backoff) },
        { "peer_pub", required_argument, 0, 'P' },
        { "my_priv", required_argument, 0, 'k' },
        { "paclen", required_argument, 0, 'l' },
        { "window", required_argument, 0, 'w' },
        { "idle", required_argument, 0, int(coptval::idle) },
        { "extseq", no_argument, 0, 'e' },
    };
    const auto dgram = DGram::common_long_opts();
    ret.insert(ret.end(), dgram.begin(), dgram.end());
    return ret;
}

std::vector<struct option> DGram::common_long_opts()
{
    std::vector<struct ::option> ret{
        { "path", required_argument, 0, 'p' },
        { "radio", required_argument, 0, 'r' },
        { "mycall", required_argument, 0, 's' },
    };
    return ret;
}

bool DGram::common_opt(CommonOpts& o, int opt)
{
    switch (opt) {
    case 'p':
        o.path = split(optarg, ',');
        break;
    case 'r': {
        const auto t = ax25_config_get_addr(optarg);
        if (t == nullptr) {
            if (errno == 0) {
                throw std::runtime_error(std::string("invalid radio \"") + optarg + "\"");
            }
            throw std::system_error(errno,
                                    std::generic_category(),
                                    std::string("invalid radio \"") + optarg + "\"");
        }
        o.radio = t;
        if (o.src.empty()) {
            o.src = o.radio;
        }
    } break;
    case 's':
        o.src = optarg;
        break;
    default:
        return false;
    }
    return true;
}

bool SeqPacket::common_opt(CommonOpts& o, int opt)
{
    const auto dgram = DGram::common_opt(o, opt);
    if (dgram) {
        return true;
    }

    switch (opt) {
    case int(coptval::t1):
        o.t1 = parse_uint(optarg);
        break;
    case int(coptval::t2):
        o.t2 = parse_uint(optarg);
        break;
    case int(coptval::t3):
        o.t3 = parse_uint(optarg);
        break;
    case int(coptval::n2):
        o.n2 = parse_uint(optarg);
        break;
    case int(coptval::backoff):
        o.backoff = parse_uint(optarg);
        break;
    case int(coptval::idle):
        o.idle = parse_uint(optarg);
        break;
    case 'e':
        o.extended_modulus = true;
        break;
    case 'w':
        o.window = parse_uint(optarg);
        break;
    case 'l':
        o.packet_length = parse_uint(optarg);
        break;
    case 'P':
        o.peer_pub = load_key<32>(optarg);
        o.peer_pub_provided = true;
        break;
    case 'k':
        o.my_priv = load_key<64>(optarg);
        o.my_priv_provided = true;
        break;
    default:
        return false;
    }
    return true;
}

std::vector<std::string> split(std::string s, char splitchar)
{
    std::stringstream ss{ s };
    std::string word;
    std::vector<std::string> ret;
    while (std::getline(ss, word, splitchar)) {
        ret.push_back(word);
    }
    return ret;
}

void populate_digis(struct full_sockaddr_ax25* sa, const std::vector<std::string>& digis)
{
    using s_t = decltype(digis.size());

    int offset = sa->fsa_ax25.sax25_ndigis;

    if (false) {
        std::clog << sa->fsa_ax25.sax25_ndigis << " digis before\n";
        for (int i = 0; i < sa->fsa_ax25.sax25_ndigis; i++) {
            std::clog << " " << ax25_ntoa(&sa->fsa_digipeater[i]) << "\n";
        }
    }

    sa->fsa_ax25.sax25_ndigis = std::min(digis.size() + offset, s_t(AX25_MAX_DIGIS));
    for (int i = 0; i < digis.size(); i++) {
        if (-1 ==
            ax25_aton_entry(digis[i].c_str(),
                            reinterpret_cast<char*>(&sa->fsa_digipeater[i + offset]))) {
            throw std::system_error(
                errno, std::generic_category(), "ax25_aton_entry(" + digis[i] + ")");
        }
    }

    if (false) {
        std::clog << sa->fsa_ax25.sax25_ndigis << " digis after\n";
        for (int i = 0; i < sa->fsa_ax25.sax25_ndigis; i++) {
            std::clog << "  " << ax25_ntoa(&sa->fsa_digipeater[i]) << "\n";
        }
    }
}

DGram::DGram(std::string radio, std::string mycall, std::vector<std::string> digipeaters)
    : sock_(socket(AF_AX25, SOCK_DGRAM, 0)),
      radio_(std::move(radio)),
      mycall_(std::move(mycall)),
      digipeaters_(std::move(digipeaters))
{
    if (sock_ == -1) {
        throw std::system_error(
            errno, std::generic_category(), "socket(AF_AX25, SOCK_DGRAM, 0)");
    }
    if (mycall_.empty()) {
        throw std::runtime_error("empty MYCALL provided to DGram");
    }

    struct full_sockaddr_ax25 me {};
    me.fsa_ax25.sax25_family = AF_AX25;
    if (-1 == ax25_aton(mycall_.c_str(), &me)) {
        throw std::system_error(
            errno, std::generic_category(), "ax25_aton(" + mycall_ + ")");
    }
    populate_digis(&me, { radio_ });
    if (-1 == bind(sock_, reinterpret_cast<struct sockaddr*>(&me), sizeof(me))) {
        throw std::system_error(
            errno, std::generic_category(), "bind to " + mycall_ + " failed");
    }
    if (false) {
        if (-1 ==
            setsockopt(sock_, SOL_AX25, SO_BINDTODEVICE, radio.c_str(), radio.size())) {
            throw std::system_error(
                errno, std::generic_category(), "setting AX25_BINDTODEVICE " + radio);
        }
    }
}

std::pair<std::string, std::string> DGram::recv()
{
    struct full_sockaddr_ax25 sa {};
    socklen_t salen = sizeof(sa);
    std::array<char, 4196> buf;
    const auto rc = recvfrom(sock_,
                             buf.data(),
                             buf.size(),
                             0,
                             reinterpret_cast<struct sockaddr*>(&sa),
                             &salen);
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "recvfrom()");
    }
    return std::make_pair("TODO", std::string(buf.data(), buf.data() + rc));
}

void DGram::write(const std::string& peer, const std::string& msg)
{
    struct full_sockaddr_ax25 sa {};
    if (-1 == ax25_aton(peer.c_str(), &sa)) {
        throw std::system_error(
            errno, std::generic_category(), "ax25_aton(" + peer + ")");
    }
    // Populate path to destination.
    populate_digis(&sa, digipeaters_);
    const auto rc = sendto(sock_,
                           msg.data(),
                           msg.size(),
                           0,
                           reinterpret_cast<struct sockaddr*>(&sa),
                           sizeof(sa));
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "sendto()");
    }
}

SeqPacket::SeqPacket(std::string radio,
                     std::string mycall,
                     std::vector<std::string> digipeaters)
    : sock_(socket(AF_AX25, SOCK_SEQPACKET, 0)),
      radio_(std::move(radio)),
      mycall_(std::move(mycall)),
      digipeaters_(std::move(digipeaters))
{
    if (sock_ == -1) {
        throw std::system_error(
            errno, std::generic_category(), "socket(AF_AX25, SOCK_SEQPACKET, 0)");
    }
    if (mycall_.empty()) {
        throw std::runtime_error("empty MYCALL provided to SeqPacket");
    }

    set_parms(sock_);

    struct full_sockaddr_ax25 me {};
    me.fsa_ax25.sax25_family = AF_AX25;
    if (-1 == ax25_aton(mycall_.c_str(), &me)) {
        throw std::system_error(
            errno, std::generic_category(), "ax25_aton(" + mycall_ + ")");
    }
    // Populate path out to the radio.
    populate_digis(&me, { radio_ });
    if (-1 == bind(sock_, reinterpret_cast<struct sockaddr*>(&me), sizeof(me))) {
        throw std::system_error(
            errno, std::generic_category(), "bind to " + mycall_ + " failed");
    }
}

int SeqPacket::connect(std::string addr)
{
    peer_addr_ = std::move(addr);
    struct full_sockaddr_ax25 peer {};
    peer.fsa_ax25.sax25_family = AF_AX25;
    if (-1 == ax25_aton_entry(peer_addr_.c_str(),
                              reinterpret_cast<char*>(&peer.fsa_ax25.sax25_call))) {
        throw std::system_error(
            errno, std::generic_category(), "ax25_aton_entry(" + mycall_ + ")");
    }
    peer.fsa_ax25.sax25_ndigis =
        std::min(digipeaters_.size(), decltype(digipeaters_)::size_type(AX25_MAX_DIGIS));
    populate_digis(&peer, digipeaters_);
    if (-1 == ::connect(sock_, reinterpret_cast<struct sockaddr*>(&peer), sizeof(peer))) {
        return errno;
    }
    set_parms(sock_);
    return 0;
}

void SeqPacket::write(const std::string& msg)
{
    ssize_t rc;
    do {
        rc = ::write(sock_, msg.data(), msg.size());
    } while (rc == -1 && errno == EINTR);
    if (static_cast<size_t>(rc) != msg.size()) {
        throw std::system_error(errno, std::generic_category(), "write()");
    }
}

void SeqPacket::write_chunked(const std::string& msg)
{
    const auto len = max_packet_size();
    const auto size = msg.size();
    const auto data = msg.data();
    for (size_t pos = 0; pos < size; pos += len) {
        const auto from = data + pos;
        const auto to = std::min(from + len, data + size);
        // TODO: check for error.
        write(std::string(from, to));
    }
}

std::string SeqPacket::read()
{
    std::array<char, 1000> buf;
    ssize_t rc;
    do {
        rc = ::read(sock_, buf.data(), buf.size());
    } while (rc == -1 && errno == EINTR);
    if (rc == -1) {
        if (errno == ENOTCONN) {
            return "";
        }
        throw std::system_error(errno, std::generic_category(), "read()");
    }
    return std::string(&buf[0], &buf[rc]);
}

void SeqPacket::listen(std::function<void(std::unique_ptr<SeqPacket>)> cb)
{
    if (-1 == ::listen(sock_, 5)) {
        throw std::system_error(errno, std::generic_category(), "listen()");
    }

    for (;;) {
        struct full_sockaddr_ax25 peer {};
        socklen_t len = sizeof(peer);
        int fd = accept(sock_, reinterpret_cast<struct sockaddr*>(&peer), &len);
        if (fd == -1) {
            std::clog << "accept(): " << strerror(errno) << "\n";
            sleep(1);
            continue;
        }
        set_parms(fd);
        auto p = reinterpret_cast<const ax25_address*>(&peer.fsa_ax25.sax25_call);
        // Private constructor, thus can't use make_unique.
        auto s =
            std::unique_ptr<SeqPacket>(new SeqPacket(radio_, mycall_, fd, ax25_ntoa(p)));
        copy_parms(*s);
        cb(std::move(s));
    }
}

void SeqPacket::set_window_size(unsigned int n)
{
    window_size_ = n;
    set_window_size_fd(sock_, window_size_);
}
void SeqPacket::set_packet_length(unsigned int n)
{
    packet_length_ = n;
    set_packet_length_fd(sock_, packet_length_);
}
void SeqPacket::set_extended_modulus(bool v)
{
    extended_modulus_ = v;
    set_extended_modulus_fd(sock_, extended_modulus_);
}
void SeqPacket::set_t1(int v)
{
    t1_ = v;
    set_int_fd(sock_, AX25_T1, t1_);
}
void SeqPacket::set_t2(int v)
{
    t2_ = v;
    set_int_fd(sock_, AX25_T2, t2_);
}
void SeqPacket::set_t3(int v)
{
    t3_ = v;
    set_int_fd(sock_, AX25_T3, t3_);
}
void SeqPacket::set_n2(int v)
{
    n2_ = v;
    set_int_fd(sock_, AX25_N2, n2_);
}
void SeqPacket::set_backoff(int v)
{
    backoff_ = v;
    set_int_fd(sock_, AX25_BACKOFF, backoff_);
}
void SeqPacket::set_idle(int v)
{
    idle_ = v;
    set_int_fd(sock_, AX25_IDLE, idle_);
}

void SeqPacket::set_parms(int fd)
{
    set_extended_modulus_fd(fd, extended_modulus_);
    set_window_size_fd(fd, window_size_);
    set_packet_length_fd(fd, packet_length_);
    set_int_fd(fd, AX25_T1, t1_);
    set_int_fd(fd, AX25_T2, t2_);
    set_int_fd(fd, AX25_T3, t3_);
    set_int_fd(fd, AX25_N2, n2_);
    set_int_fd(fd, AX25_BACKOFF, backoff_);
    set_int_fd(fd, AX25_IDLE, idle_);
}

void SeqPacket::copy_parms(SeqPacket& other) const noexcept
{
    other.set_window_size(window_size_);
    other.set_extended_modulus(extended_modulus_);
    other.set_packet_length(packet_length_);
    other.set_t1(t1_);
    other.set_t2(t2_);
    other.set_t3(t3_);
    other.set_n2(n2_);
    other.set_backoff(backoff_);
    other.set_idle(idle_);
}

unsigned int SeqPacket::window_size() const
{
    unsigned int ret;
    socklen_t len = sizeof(ret);
    if (-1 == getsockopt(sock_, SOL_AX25, AX25_WINDOW, &ret, &len)) {
        throw std::system_error(errno, std::generic_category(), "getting AX25_WINDOW");
    }
    return ret;
}

unsigned int SeqPacket::packet_length() const
{
    unsigned int ret;
    socklen_t len = sizeof(ret);
    if (-1 == getsockopt(sock_, SOL_AX25, AX25_PACLEN, &ret, &len)) {
        throw std::system_error(errno, std::generic_category(), "getting AX25_PACLEN");
    }
    return ret;
}

void Sock::close() noexcept
{
    if (fd != -1) {
        ::close(fd);
        fd = -1;
    }
}

Sock::Sock(Sock&& rhs) noexcept { *this = std::move(rhs); }
Sock& Sock::operator=(Sock&& rhs) noexcept
{
    fd = rhs.fd;
    rhs.fd = -1;
    return *this;
}

SignedSeqPacket::SignedSeqPacket(SeqPacket&& rhs,
                                 std::array<char, 64> priv,
                                 std::array<char, 32> pub)
    : SeqPacket(std::move(rhs)), my_priv_(priv), peer_pub_(pub)
{}

void SignedSeqPacket::listen(std::function<void(std::unique_ptr<SeqPacket>)> cb)
{
    SeqPacket::listen([&cb, this](std::unique_ptr<SeqPacket> s) {
        auto ss = std::unique_ptr<SeqPacket>(
            std::make_unique<SignedSeqPacket>(std::move(*s), my_priv_, peer_pub_));
        try {
            dynamic_cast<SignedSeqPacket*>(ss.get())->exchange_nonce();
            cb(std::move(ss));
        } catch (const std::exception& e) {
            std::clog << "Connection failed with exception: " << e.what() << std::endl;
        }
    });
}

void SignedSeqPacket::write(const std::string& msg)
{
    SeqPacket::write(sign(msg) + msg);
    packets_sent_++;
}

void SignedSeqPacket::exchange_nonce()
{
    if (packet_good_ > 0 || packet_bad_ > 0) {
        return;
    }
    std::vector<char> n(nonce_size_);
    const auto rc = getrandom(&n[0], n.size(), 0);
    if (rc == -1) {
        throw std::system_error(errno, std::generic_category(), "getrandom()");
    }
    if (static_cast<size_t>(rc) != n.size()) {
        throw std::runtime_error("too few random bytes(): " + std::to_string(rc) + "<" +
                                 std::to_string(n.size()));
    }
    const std::string local(std::begin(n), std::end(n));
    write(local);
    nonce_peer_ = read();
    nonce_local_ = local;
}

int SignedSeqPacket::connect(std::string addr)
{
    int ret = SeqPacket::connect(std::move(addr));
    if (ret) {
        return ret;
    }
    exchange_nonce();
    return ret;
}

std::string str2hex(const std::string& data)
{
    std::stringstream ss;
    for (auto ch : data) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << (static_cast<unsigned int>(ch) & 0xff) << " ";
    }
    return ss.str();
}

std::string SignedSeqPacket::read()
{
    const auto full = SeqPacket::read();
    if (full.empty()) {
        return "";
    }
    const auto msg = full.substr(sig_size_);
    const auto sig = full.substr(0, sig_size_);
    if (!crypto::verify(full + nonce_peer_ + nonce_local_ + std::to_string(packet_good_),
                        peer_pub_)) {
        packet_bad_++;
        throw std::runtime_error("bad signature on packet " +
                                 std::to_string(packet_good_) + ": " + str2hex(full));
    }
    packet_good_++;
    return msg;
}

std::string SignedSeqPacket::sign(const std::string& msg) const
{
    const auto ret = crypto::sign(
        msg + nonce_local_ + nonce_peer_ + std::to_string(packets_sent_), my_priv_);
    if (ret.size() != sig_size_) {
        throw std::runtime_error("signature wrong size(): " + std::to_string(ret.size()));
    }
    return ret;
}

template <int size>
std::array<char, size> load_key(const std::string& fn)
{
    std::array<char, size> buf;
    std::ifstream pf(fn);
    if (pf.good()) {
        pf.read(buf.data(), buf.size());
    }
    if (pf.fail()) {
        throw std::system_error(errno,
                                std::generic_category(),
                                "loading key of size " + std::to_string(size) +
                                    " from file '" + fn + "'");
    }
    return buf;
}

template std::array<char, 32> load_key<32>(const std::string& fn);
template std::array<char, 64> load_key<64>(const std::string& fn);

} // namespace axlib
