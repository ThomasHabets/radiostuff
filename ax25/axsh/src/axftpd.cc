#include "axlib.h"

#include <functional>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <thread>

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

using namespace axlib;

namespace {

constexpr int max_buf_size = 1000;

void handle_get(SeqPacket* conn, const std::vector<std::string>& args)
{
    if (args.size() != 2) {
        conn->write("ERR 'get' command takes exactly one arg");
        return;
    }

    const auto fn = args[1];
    const std::regex fnRE("^[^/]+$");
    if (!std::regex_match(fn, fnRE)) {
        conn->write("ERR invalid filename: " + fn);
        return;
    }

    FILE* f = fopen(fn.c_str(), "r");
    // TODO: leaks filehandle if there's an exception.
    if (f == nullptr) {
        conn->write("ERR failed to open " + fn + ": " + strerror(errno));
        return;
    }
    if (fseek(f, 0, SEEK_END)) {
        const auto err = errno;
        fclose(f);
        conn->write("ERR failed to seek to end in " + fn + ": " + strerror(err));
        return;
    }
    const auto size = ftell(f);
    if (size == -1) {
        const auto err = errno;
        fclose(f);
        conn->write("ERR failed to ftell() at end in " + fn + ": " + strerror(err));
        return;
    }
    if (fseek(f, 0, SEEK_SET)) {
        const auto err = errno;
        fclose(f);
        conn->write("ERR failed to seek to start in " + fn + ": " + strerror(err));
        return;
    }
    conn->write(std::to_string(size));
    const auto len = conn->max_packet_size();
    std::clog << "Handling connection with packet length " << len << std::endl;
    for (;;) {
        std::vector<char> buf(len);
        const auto n = fread(&buf[0], 1, buf.size(), f);
        if (n == 0) {
            if (feof(f)) {
                break;
            }
            if (ferror(f)) {
                const auto err = errno;
                fclose(f);
                throw std::system_error(err, std::generic_category(), "reading stdin");
            }
        }
        std::clog << "Sending packet of size " << n << std::endl;
        conn->write(std::string(&buf[0], &buf[n]));
    }
    fclose(f);
}

void handle_list(SeqPacket* conn, [[maybe_unused]] const std::vector<std::string>& args)
{
    std::string buf;
    auto dir = opendir(".");
    if (dir == nullptr) {
        throw std::system_error(errno, std::generic_category(), "opendir(.)");
    }
    while (auto f = readdir(dir)) {
        if (f->d_name[0] == '.') {
            continue;
        }
        switch (f->d_type) {
        case DT_DIR:
            buf += f->d_name + std::string("/\n");
            break;
        case DT_REG:
            buf += f->d_name + std::string("\n");
            break;
        }
    }
    closedir(dir);
    conn->write(std::to_string(buf.size()));
    conn->write_chunked(buf);
}

void handle2(std::unique_ptr<SeqPacket> conn)
{
    std::string buf;
    for (;;) {
        // Get line.
        if (buf.size() > max_buf_size) {
            throw std::runtime_error("client sent too much data");
        }
        buf += conn->read();
        const auto nl = buf.find('\n');
        if (nl == std::string::npos) {
            continue;
        }
        const auto line = buf.substr(0, nl);
        buf = buf.substr(nl + 1);

        // Parse command.
        const auto args = axlib::split(line, ' ');

        if (args[0] == "list") {
            handle_list(conn.get(), args);
        } else if (args[0] == "get") {
            handle_get(conn.get(), args);
        } else {
            conn->write("ERR invalid command <" + args[0] + ">");
        }
    }
    std::clog << "closing connection\n";
}

void handle(std::unique_ptr<SeqPacket> conn)
{
    try {
        handle2(std::move(conn));
    } catch (const std::exception& e) {
        std::cerr << "Exception in client handler: " << e.what() << std::endl;
    }
}

[[noreturn]] void usage(const char* av0, int err)
{
    FILE* f = stdout;
    if (err) {
        f = stderr;
    }
    fprintf(f,
            "Usage: %s […options…] -r <radio> -s <src call>\n"
            "%s\nExample:\n"
            "   %s -r radio6 -k my.priv -P peer.pub -s M0XXX-9 -p M0XXX-0\n",
            av0,
            SeqPacket::common_usage().c_str(),
            av0);
    exit(err);
}

} // namespace

int wrapmain(int argc, char** argv)
{
    CommonOpts copt;
    int opt;
    auto lopts = SeqPacket::common_long_opts();
    common_init();
    lopts.push_back({ 0, 0, 0, 0 });
    while ((opt = getopt_long(argc, argv, "ehk:l:P:p:r:s:w:", &lopts[0], NULL)) != -1) {
        if (SeqPacket::common_opt(copt, opt)) {
            continue;
        }
        switch (opt) {
        case 'h':
            usage(argv[0], EXIT_SUCCESS);
        default: /* '?' */
            usage(argv[0], EXIT_FAILURE);
        }
    }

    if (optind != argc) {
        fprintf(stderr, "Extra args on command line\n");
        exit(EXIT_FAILURE);
    }

    auto sock = SeqPacket::make_from_commonopts(copt);

    std::clog << "Listening...\n";
    std::vector<std::thread> threads;
    sock->listen([&threads](std::unique_ptr<SeqPacket> conn) {
        std::clog << "Connection from " << conn->peer_addr() << "\n";
        threads.emplace_back(handle, std::move(conn));
    });
    for (auto& th : threads) {
        th.join();
    }
    return 0;
}
