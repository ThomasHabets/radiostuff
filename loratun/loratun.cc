// C++
#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// POSIX
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

// Linux
#include <linux/if.h>
#include <linux/if_tun.h>

namespace {
namespace options {
int sf = 12;            // 7-12, lower is faster
int bw = 125;           // 125,250,500, higher is faster
const char* cr = "4/8"; // 4/5, 4/6, 4/7, 4/8
bool persist = true;
} // namespace options
} // namespace


void full_write(int fd, const std::string& s)
{
    // TODO: full write
    ::write(fd, s.data(), s.size());
}

std::string readline(int fd)
{
    std::vector<char> ret;
    for (;;) {
        char buf;
        // TODO: check error
        struct pollfd fds {
            .fd = fd, .events = POLLIN,
        };
        poll(&fds, 1, -1);
        const auto rc = ::read(fd, &buf, 1);
        if (rc == 0) {
            continue;
        }
        if (rc != 1) {
            throw std::runtime_error(std::string{ "read()=" } + std::to_string(rc) +
                                     ": " + strerror(errno));
        }
        if (buf == '\n') {
            return std::string(ret.begin(), ret.end());
        }
        if (buf != '\r') {
            ret.push_back(buf);
        }
    }
}

class LoStik
{
public:
    LoStik(std::string dev);
    std::string cmd(const std::string&);
    std::string readline();
    int get_fd() const noexcept { return fd_; }

private:
    void flush();
    const std::string dev_;
    int fd_ = -1;

    static constexpr int speed = B57600;

    // TODO: create a send() that takes pending_tx into account.
    bool pending_tx_ = false;
};

LoStik::LoStik(std::string dev) : dev_(dev)
{
    fd_ = open(dev.c_str(), O_RDWR);
    if (fd_ == -1) {
        throw std::runtime_error(std::string{ "open(): " } + strerror(errno));
    }

    // Set serial parameters.
    struct termios tty;
    if (tcgetattr(fd_, &tty)) {
        close(fd_);
        throw std::runtime_error("tcgetattr(" + dev + "): " + strerror(errno));
    }
    tty.c_cflag = CREAD | CLOCAL | CS8 | HUPCL;
    tty.c_lflag = 0;
    tty.c_iflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN] = 0;  // No block.
    tty.c_cc[VTIME] = 0; // 0 read timeout for noncanonical read.
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    if (tcsetattr(fd_, TCSANOW, &tty)) {
        close(fd_);
        throw std::runtime_error("tcsetattr(" + dev + "): " + strerror(errno));
    }

    flush();
    std::clog << "LoStik ver: " << cmd("sys get ver") << std::endl
              << "Setting options…\n"
              << "… Set mac pause: " << cmd("mac pause") << std::endl
              << "… Stop RX: " << cmd("radio rxstop") << std::endl
              << "… Set power: " << cmd("radio set pwr 15") << std::endl
              << "… SF: " << cmd("radio set sf sf" + std::to_string(options::sf))
              << std::endl
              << "… Bandwidth: " << cmd("radio set bw " + std::to_string(options::bw))
              << std::endl
              << "… Set watchdog: " << cmd("radio set wdt 0") << std::endl
              << "… CRC: " << cmd("radio set crc on") << std::endl
              << "… CR: " << cmd(std::string{ "radio set cr " } + options::cr)
              << std::endl
              << "… LED 0 off: " << cmd("sys set pindig GPIO10 0") << std::endl
              << "… LED 1 off: " << cmd("sys set pindig GPIO11 0") << std::endl;

    std::clog << "Settings:\n"
              << "… Power: " << cmd("radio get pwr") << std::endl
              << "… Modulation: " << cmd("radio get mod") << std::endl
              << "… Frequency: " << cmd("radio get freq") << std::endl
              << "… SF: " << cmd("radio get sf") << std::endl
              << "… Bandwidth: " << cmd("radio get bw") << std::endl
              << "… VDD: " << cmd("sys get vdd") << std::endl
              << "… EUI: " << cmd("sys get hweui") << std::endl
              << "… CRC: " << cmd("radio get crc") << std::endl
              << "… CR: " << cmd("radio get cr") << std::endl
              << "… Sync: " << cmd("radio get sync") << std::endl
              << "… Watchdog: " << cmd("radio get wdt") << std::endl;
}

void LoStik::flush()
{
    struct pollfd fds {
        .fd = fd_, .events = POLLIN,
    };
    std::clog << "Flushing...\n";
    for (;;) {
        std::array<char, 128> buf;
        const auto ret = poll(&fds, 1, 0);
        if (ret == 0) {
            return;
        }
        ::read(fd_, buf.data(), buf.size());
    }
}

std::string LoStik::cmd(const std::string& cmd)
{
    full_write(fd_, cmd + "\r\n");
    return readline();
}

std::string LoStik::readline()
{
    const auto ret = ::readline(fd_);
    if (ret == "radio_tx_ok") {
        cmd("sys set pindig GPIO11 0");
        pending_tx_ = false;
    }
    return ret;
}


std::string to_hex(const std::string& s)
{
    const char* hexes = "0123456789ABCDEF";
    std::vector<char> ret;
    for (auto ch : s) {
        ret.push_back(hexes[(ch >> 4) & 0xf]);
        ret.push_back(hexes[ch & 0xf]);
    }
    return std::string(ret.begin(), ret.end());
}

char nibble(char ch)
{
    if (isdigit(ch)) {
        return ch - '0';
    }
    return 10 + (ch - 'A');
}

std::string from_hex(const std::string& s)
{
    if (s.size() & 1) {
        throw std::runtime_error("from_hex() called with odd size");
    }
    const char* hexes = "0123456789ABCDEF";
    std::vector<char> ret;
    for (int i = 0; i < s.size(); i += 2) {
        ret.push_back((nibble(s[i]) << 4) + nibble(s[i + 1]));
    }
    return std::string(ret.begin(), ret.end());
}

void receive(int fd, LoStik& stick)
{
    const auto line = stick.readline();
    const std::string prefix{ "radio_rx  " };
    if (!strncmp(prefix.c_str(), line.c_str(), prefix.size())) {
        const auto packet = std::string(&line.c_str()[prefix.size()]);
        std::cout << "packet: <" << packet << ">\n";
        const auto p = from_hex(packet);
        std::cout << "… snr:  " << stick.cmd("radio get snr") << std::endl;
        ::write(fd, p.data(), p.size());
        std::cout << "reset radio_rx reply> " << stick.cmd("radio rx 0") << std::endl;
        return;
    }

    if (line == "radio_tx_ok") {
        std::cout << "Packet sent\n";
        std::cout << "... restarting listen> " << stick.cmd("radio rx 0") << std::endl;
        return;
    }

    std::cout << "What? no match!? <" << line << ">\n";
    std::cout << "reset radio_rx reply> " << stick.cmd("radio rx 0") << std::endl;
}

// tx on lostik
void send(int fd, LoStik& stick)
{
    std::array<char, 4096> buf;
    ssize_t n;
    struct pollfd fds {
        .fd = fd, .events = POLLIN,
    };
    poll(&fds, 1, -1);
    if (0 > (n = read(fd, buf.data(), buf.size()))) {
        throw std::runtime_error(std::string{ "read(): " } + strerror(errno));
    }
    const auto msg = to_hex(std::string(&buf[0], &buf[n]));

    // std::cout << "rx stop>" << stick.cmd("rxstop") << std::endl;
    std::cout << "rx stop>" << stick.cmd("radio rxstop") << std::endl;
    auto sbuf = std::string{ "radio tx " } + msg;
    stick.cmd("sys set pindig GPIO11 1");
    std::cout << "sending packet...<" << sbuf << ">\n";
    const auto reply = stick.cmd(sbuf);
    std::clog << "... " << reply << std::endl;
    // std::clog << "... " << stick.await("radio_tx_ok") << std::endl;
    // std::clog << "... " << stick.readline() << std::endl;
    usleep(10000);
}

int mainwrap(int argc, char** argv)
{
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd == -1) {
        throw std::runtime_error(std::string{ "open(): " } + strerror(errno));
    }
    struct ifreq ifr {
        .ifr_flags = IFF_TUN,
    };
    memset(&ifr, 0, sizeof(struct ifreq));
    ifr.ifr_flags = IFF_TUN;
    const std::string dev = argv[1];
    const std::string tundev = argv[2];
    strncpy(ifr.ifr_name, tundev.c_str(), IFNAMSIZ);
    if (0 > ioctl(fd, TUNSETIFF, reinterpret_cast<void*>(&ifr))) {
        throw std::runtime_error(std::string{ "ioctl(): " } + strerror(errno));
    }
    if (options::persist) {
        int on = 1;
        if (0 > ioctl(fd, TUNSETPERSIST, reinterpret_cast<void*>(&on))) {
            throw std::runtime_error(std::string{ "ioctl(TUNSETPERSIST): " } +
                                     strerror(errno));
        }
    }


    LoStik stick{ dev };
    std::cout << "radio rx reply> " << stick.cmd("radio rx 0") << std::endl;
    for (;;) {
        struct pollfd fds[] = { {
                                    .fd = fd,
                                    .events = POLLIN,
                                },
                                {
                                    .fd = stick.get_fd(),
                                    .events = POLLIN,
                                } };
        poll(fds, 2, -1);
        if (fds[0].revents & POLLIN) {
            send(fd, stick);
        }
        if (fds[1].revents & POLLIN) {
            receive(fd, stick);
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    try {
        return mainwrap(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
