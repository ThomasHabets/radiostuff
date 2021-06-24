#include "loralib.h"

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
  const auto rc = ::write(fd, s.data(), s.size());
  if (rc != s.size()) {
    throw std::runtime_error("full_write()");
  }
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

bool LoStik::send(const std::vector<unsigned char>& vec)
{
    if (send_in_progress_) {
        return false;
    }
    const auto msg = to_hex(std::string(vec.begin(), vec.end()));
    // std::cout << "rx stop>" << cmd("rxstop") << std::endl;
    std::cout << "rx stop>" << cmd("radio rxstop") << std::endl;
    auto sbuf = std::string{ "radio tx " } + msg;
    cmd("sys set pindig GPIO11 1");
    std::cout << "sending packet...<" << sbuf << ">\n";
    const auto reply = cmd(sbuf);
    std::clog << "... " << reply << std::endl;
    usleep(10000); // Give the receiver a chance to go back to rx mode.
    if (reply == "ok") {
        send_in_progress_ = true;
        return true;
    }
    return false;
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

std::vector<unsigned char> LoStik::process()
{
    const auto line = readline();
    const std::string prefix{ "radio_rx  " };
    if (!strncmp(prefix.c_str(), line.c_str(), prefix.size())) {
        const auto packet = std::string(&line.c_str()[prefix.size()]);
        std::cout << "packet: <" << packet << ">\n";
        const auto p = from_hex(packet);
        std::cout << "… snr:  " << cmd("radio get snr") << std::endl;
        std::cout << "reset radio_rx reply> " << cmd("radio rx 0") << std::endl;
        return std::vector<unsigned char>(p.begin(), p.end());
    }

    if (line == "radio_tx_ok") {
        send_in_progress_ = false;
        std::cout << "Packet sent\n";
        std::cout << "... restarting listen> " << cmd("radio rx 0") << std::endl;
        return {};
    }

    std::cout << "What? no match!? <" << line << ">\n";
    std::cout << "reset radio_rx reply> " << cmd("radio rx 0") << std::endl;
    return {};
}
