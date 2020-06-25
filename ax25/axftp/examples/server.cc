#include "../src/axlib.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <mycall> <path>\n";
        return EXIT_FAILURE;
    }

    axlib::SeqPacket sock(argv[1], axlib::split(argv[2]));

    std::cerr << "Listening...\n";
    sock.listen([](auto conn) {
        std::cout << conn->read() << std::endl;
        conn->write("yo");
    });
}
