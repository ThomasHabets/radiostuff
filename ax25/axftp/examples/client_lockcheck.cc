#include "../src/axlib.h"

#include <unistd.h>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <radio> <mycall> <path> <dest>\n";
        return EXIT_FAILURE;
    }
    const std::string radio = argv[1];
    const std::string mycall = argv[2];
    const std::string path = argv[3];
    const std::string dest = argv[4];
    axlib::SeqPacket sock(radio, mycall, axlib::split(path));

    sock.connect(dest);
    std::thread th([&] { std::cout << sock.read() << std::endl; });
    sleep(1);
    sock.write("hello world");
    th.join();
}
