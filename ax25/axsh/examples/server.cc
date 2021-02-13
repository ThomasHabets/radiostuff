#include "../src/axlib.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <radio> <mycall> <path>\n";
        return EXIT_FAILURE;
    }
    const std::string radio = argv[1];
    const std::string mycall = argv[2];
    const std::string path = argv[3];

    axlib::SeqPacket sock(radio, mycall, axlib::split(path, ','));

    std::cerr << "Listening...\n";
    sock.listen([](auto conn) {
        std::cout << conn->read() << std::endl;
        conn->write("yo");
    });
}
