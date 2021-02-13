#include <sodium/core.h>
#include <sodium/crypto_sign.h>

#include <fstream>
#include <iostream>
#include <vector>

#include <getopt.h>
#include <unistd.h>

void usage(const char* av0, int e)
{
    std::cerr << "Usage: " << av0 << "[options] <keybase>\n";
    exit(e);
}
int main(int argc, char** argv)
{
    std::vector<struct ::option> lopts{};
    int opt;
    while ((opt = getopt_long(argc, argv, "h", &lopts[0], NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage(argv[0], EXIT_SUCCESS);
        default: /* '?' */
            usage(argv[0], EXIT_FAILURE);
        }
    }

    if (argc != optind + 1) {
        usage(argv[0], EXIT_FAILURE);
    }
    const std::string base = argv[optind];
    if (sodium_init() == -1) {
        throw std::runtime_error("sodium_init() failed");
    }
    char pk[crypto_sign_PUBLICKEYBYTES];
    char sk[crypto_sign_SECRETKEYBYTES];
    crypto_sign_keypair(reinterpret_cast<unsigned char*>(pk),
                        reinterpret_cast<unsigned char*>(sk));

    // Do a signature test.
    {
        const std::string msg{ "test" };
        unsigned char sig[crypto_sign_BYTES];
        crypto_sign_detached(sig,
                             NULL,
                             reinterpret_cast<const unsigned char*>(msg.data()),
                             msg.size(),
                             reinterpret_cast<unsigned char*>(sk));

        if (crypto_sign_verify_detached(
                sig,
                reinterpret_cast<const unsigned char*>(msg.data()),
                msg.size(),
                reinterpret_cast<unsigned char*>(pk)) != 0) {
            /* Incorrect signature! */
            throw std::runtime_error("self check failed");
        }
    }

    {
        const auto fn = base + ".pub";
        std::ofstream pub(fn);
        pub.write(pk, crypto_sign_PUBLICKEYBYTES);
        pub.close();
        if (!pub.good()) {
            throw std::runtime_error("opening " + fn + " failed");
        }
    }

    {
        const auto fn = base + ".priv";
        std::ofstream priv(fn);
        priv.write(sk, crypto_sign_SECRETKEYBYTES);
        priv.close();
        if (!priv.good()) {
            throw std::runtime_error("opening " + fn + " failed");
        }
    }
}
