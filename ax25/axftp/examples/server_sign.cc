#include "../src/axlib.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <radio> <mycall> <path> <priv> <pub>\n";
        return EXIT_FAILURE;
    }
    const std::string radio = argv[1];
    const std::string mycall = argv[2];
    const std::string path = argv[3];
    const std::string priv = argv[4];
    const std::string pub = argv[5];

    axlib::SignedSeqPacket sock(radio,
                                mycall,
                                axlib::load_key<64>(priv),
                                axlib::load_key<32>(pub),
                                axlib::split(path));

    std::cerr << "Listening...\n";
    sock.listen([](auto conn) {
        std::cout << conn->read() << std::endl;
        conn->write("yo");
    });
}
