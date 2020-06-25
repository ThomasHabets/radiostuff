#include "../src/axlib.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <mycall> <path> <priv> <pub> <dest>\n";
        return EXIT_FAILURE;
    }
    axlib::SignedSeqPacket sock(argv[1],
                          axlib::load_key<64>(argv[3]),
                          axlib::load_key<32>(argv[4]),
                          axlib::split(argv[2]));

    sock.connect(argv[5]);
    sock.write("hello world");
    std::cout << sock.read() << std::endl;
}
