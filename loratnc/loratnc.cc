#include <poll.h>
#include <pty.h>
#include <unistd.h>
#include <array>
#include <climits>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "loralib.h"

constexpr unsigned char FEND = 0xC0;
constexpr unsigned char FESC = 0xDB;
constexpr unsigned char TFEND = 0xDC;
constexpr unsigned char TFESC = 0xDD;

struct StateMachine {
    using handle_ret_t = std::pair<bool, std::unique_ptr<StateMachine>>;
    using args_t = struct {
        std::vector<unsigned char>& buf;
        unsigned char ch;
        std::unique_ptr<StateMachine> self;
    };
    virtual handle_ret_t handle(args_t args) = 0;
};

struct StateREADING : public StateMachine {
    handle_ret_t handle(args_t args) override;
};

struct StateFrameEscape : public StateMachine {
    handle_ret_t handle(args_t args) override;
};

struct StateUNSYNC : public StateMachine {
    handle_ret_t handle(args_t args) override
    {
        switch (args.ch) {
        case FEND:
            return { false, std::make_unique<StateREADING>() };
        }
        return handle_ret_t(false, std::move(args.self));
    }
};


StateFrameEscape::handle_ret_t StateREADING::handle(args_t args)
{
    switch (args.ch) {
    case FEND:
        return { true, std::make_unique<StateREADING>() };
    case FESC: // Frame escape
        return { false, std::make_unique<StateFrameEscape>() };
    }
    args.buf.push_back(args.ch);
    return handle_ret_t(false, std::move(args.self));
}
StateFrameEscape::handle_ret_t StateFrameEscape::handle(args_t args)
{
    switch (args.ch) {
    case TFESC:
        args.buf.push_back(FESC);
        return { false, std::make_unique<StateREADING>() };
    case TFEND:
        args.buf.push_back(FEND);
        return { false, std::make_unique<StateREADING>() };
    default:
        std::cerr << "Invalid frame escape\n";
        return { false, std::make_unique<StateUNSYNC>() };
    }
}

void receive(int fd, LoStik& stick)
{
    const auto packet = stick.process();
    std::cerr << "Got packet of len " << packet.size() << "\n";
    auto w = [fd](unsigned char ch) {
        const auto rc = write(fd, &ch, 1);
        if (rc != 1) {
            throw std::runtime_error("write failed");
        }
    };
    for (const auto ch : packet) {
        switch (ch) {
        case FEND:
            w(FESC);
            w(TFEND);
            break;
        case FESC:
            w(FESC);
            w(TFESC);
            break;
        default:
            w(ch);
        }
    }
    w(FEND);
}

int mainwrap(int argc, char** argv)
{
    int master = -1;
    int slave = -1;
    std::array<char, PATH_MAX> name;
    if (-1 == openpty(&master, &slave, name.data(), nullptr, nullptr)) {
        throw std::runtime_error("openpty()");
    }
    std::string dev = argv[1];
    LoStik stick{ dev };
    std::cout << "radio rx reply> " << stick.cmd("radio rx 0") << std::endl;

    std::unique_ptr<StateMachine> kiss_state = std::make_unique<StateUNSYNC>();
    std::vector<unsigned char> buf;
    std::cout << name.data() << "\n";
    for (;;) {
        struct pollfd fds[] = { {
                                    .fd = master,
                                    .events = POLLIN,
                                },
                                {
                                    .fd = stick.get_fd(),
                                    .events = POLLIN,
                                } };
        if (stick.sending_in_progress()) {
            fds[0].events = 0;
        }
        poll(fds, 2, -1);
        if (fds[0].revents & POLLIN) {
            unsigned char ch;
            const auto rc = read(master, &ch, 1);
            if (rc == 0) {
                // False poll?
                continue;
            }
            if (rc == -1) {
                throw std::runtime_error("read()");
            }
            auto ptr = kiss_state.get();
            auto resp = ptr->handle({ buf, ch, std::move(kiss_state) });
            if (resp.first) {
                if (!buf.empty()) {
                    if (!stick.send(buf)) {
                        std::clog << "Sending failed!\n";
                    }
                    buf.clear();
                }
            }
            kiss_state = std::move(resp.second);
        }
        if (fds[1].revents & POLLIN) {
            receive(master, stick);
        }
    }
}

int main(int argc, char** argv)
{
    try {
        return mainwrap(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
