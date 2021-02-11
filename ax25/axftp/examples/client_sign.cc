#include "../src/axlib.h"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0]
                  << " <radio> <mycall> <path> <priv> <pub> <dest>\n";
        return EXIT_FAILURE;
    }
    const std::string radio = argv[1];
    const std::string mycall = argv[2];
    const std::string path = argv[3];
    const std::string priv = argv[4];
    const std::string pub = argv[5];
    const std::string dest = argv[6];

    axlib::SignedSeqPacket sock(radio,
                                mycall,
                                axlib::load_key<64>(priv),
                                axlib::load_key<32>(pub),
                                axlib::split(path));

    sock.connect(dest);
    sock.write("hello world");
    std::cout << sock.read() << std::endl;
}
