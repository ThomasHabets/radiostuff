#include "../src/axlib.h"

#include <iostream>

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
    sock.write("hello world");
    std::cout << sock.read() << std::endl;
}
