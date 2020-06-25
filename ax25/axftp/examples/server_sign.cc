#include "../src/axlib.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <mycall> <path> <priv> <pub>\n";
        return EXIT_FAILURE;
    }

    axlib::SignedSeqPacket sock(argv[1],
                                axlib::load_key<64>(argv[3]),
                                axlib::load_key<32>(argv[4]),
                                axlib::split(argv[2]));

    std::cerr << "Listening...\n";
    sock.listen([](auto conn) {
        std::cout << conn->read() << std::endl;
        conn->write("yo");
    });
}
