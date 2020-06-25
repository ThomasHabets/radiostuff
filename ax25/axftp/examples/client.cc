#include "../src/axlib.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <mycall> <path> <dest>\n";
        return EXIT_FAILURE;
    }
    axlib::SeqPacket sock(argv[1], axlib::split(argv[2]));

    sock.connect(argv[3]);
    sock.write("hello world");
    std::cout << sock.read() << std::endl;
}
