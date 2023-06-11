// Tool to shuffle data through D-Star, removing XON/XOFF.
//
// TODO: rate limit to 950bps/3840bps
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <termios.h>
#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <tuple>

constexpr uint8_t XOFF = 0x13;
constexpr uint8_t XON = 0x11;
constexpr uint8_t ESC = 0xDC;

constexpr uint8_t ESC_XOFF = 0x01;
constexpr uint8_t ESC_XON = 0x02;
constexpr uint8_t ESC_ESC = 0x03;

std::string encode(const std::string in)
{
    std::string ret;
    for (const auto ch : in) {
        switch (static_cast<uint8_t>(ch)) {
        case XOFF:
            ret.push_back(ESC);
            ret.push_back(ESC_XOFF);
            break;
        case XON:
            ret.push_back(ESC);
            ret.push_back(ESC_XON);
            break;
        case ESC:
            ret.push_back(ESC);
            ret.push_back(ESC_ESC);
            break;
        default:
            ret.push_back(ch);
        }
    }
    return ret;
}

std::pair<std::string, bool> decode(const std::string in, bool in_esc)
{
    std::string ret;
    for (const auto ch : in) {
        if (!in_esc) {
            switch (static_cast<uint8_t>(ch)) {
            case ESC:
                in_esc = true;
                continue;
            }
            ret.push_back(ch);
            continue;
        }

        switch (static_cast<uint8_t>(ch)) {
        case ESC_XON:
            ret.push_back(XON);
            break;
        case ESC_XOFF:
            ret.push_back(XOFF);
            break;
        case ESC_ESC:
            ret.push_back(ESC);
            break;
        default:
            std::cerr << "Input error: Invalid escape\n";
        }
        in_esc = false;
    }
    return { ret, in_esc };
}

std::pair<ssize_t, bool> copy(int src, int dst, bool enc, bool state)
{
    char buf[128];
    const auto rc = read(src, buf, sizeof buf);
    if (rc == -1) {
        perror("read()");
        return { -1, state };
    }
    if (rc) {
        const std::string in(buf, buf + rc);
        std::string data;
        if (enc) {
            data = encode(in);
        } else {
            std::tie(data, state) = decode(in, state);
        }
        const auto wsc = write(dst, data.data(), data.size());
        if (wsc == -1) {
            perror("write()");
            return { -1, state };
        }
        if (data.size() != wsc) {
            std::cerr << "short write\n";
            return { -1, state };
        }
    }
    return { rc, state };
}

// return false on error, true on clean exit.
bool mainloop(int child, int parent)
{
    bool child_state = false;
    bool parent_state = false;
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);

        FD_SET(child, &rfds);
        FD_SET(parent, &rfds);
        const auto src =
            select(std::max(child, parent) + 1, &rfds, nullptr, nullptr, nullptr);
        if (src == -1) {
            perror("select()");
            return false;
        }
        if (FD_ISSET(child, &rfds)) {
            ssize_t n;
            std::tie(n, child_state) = copy(child, parent, true, child_state);
            switch (n) {
            case 0:
                return true;
            case -1:
                return false;
            }
        }
        if (FD_ISSET(parent, &rfds)) {
            ssize_t n;
            std::tie(n, parent_state) = copy(parent, child, false, parent_state);
            switch (n) {
            case 0:
                return true;
            case -1:
                return false;
            }
        }
    }
}

int main(int argc, char** argv)
{
    assert(encode("hello\x13world") == "hello\xDC\x01world");
    assert(encode("hello\xDCworld") == "hello\xDC\x03world");

    assert(decode("hello\xDC\x01world", false).first == "hello\x13world");
    assert(decode("hello\xDC\x03world", false).first == "hello\xDCworld");
    assert(decode("hello\xDC", false).first == "hello");
    assert(decode("hello\xDC", false).second == true);
    assert(decode("\x01world", true).first == "\x13world");

    assert(decode("\x03world", true).first == "\xDCworld");

    int child = posix_openpt(O_RDWR | O_NOCTTY);
    if (child == -1) {
        perror("posix_openpt()");
        exit(1);
    }
    {
        struct termios ts;
        if (tcgetattr(child, &ts)) {
            perror("tcgetattr(child)");
            exit(1);
        }
        cfmakeraw(&ts);
        ts.c_cc[VMIN] = 1;
        ts.c_cc[VTIME] = 0;
        if (cfsetspeed(&ts, B9600)) {
            perror("tcsetspeed(child, 9600)");
            exit(1);
        }
        if (tcsetattr(child, TCSANOW, &ts)) {
            perror("tcsetattr(child)");
            exit(1);
        }
    }
    const std::string pts = ptsname(child);
    std::cout << pts << "\n";

    grantpt(child);
    unlockpt(child);
    open(pts.c_str(), O_RDWR); // TODO

    const std::string parent_tty = argv[1];
    int parent = open(parent_tty.c_str(), O_RDWR);
    if (parent == -1) {
        perror("open(parent)");
        exit(1);
    }
    {
        struct termios ts;
        if (tcgetattr(parent, &ts)) {
            perror("tcgetattr(child)");
            exit(1);
        }
        cfmakeraw(&ts);
        if (cfsetspeed(&ts, B9600)) {
            perror("tcsetspeed(parent, 9600)");
            exit(1);
        }
        ts.c_iflag |= IXON | IXOFF;
        ts.c_cc[VMIN] = 1;
        ts.c_cc[VTIME] = 0;
        if (tcsetattr(parent, TCSANOW, &ts)) {
            perror("tcsetattr(child)");
            exit(1);
        }
    }

    if (!mainloop(child, parent)) {
        return 1;
    }
}
