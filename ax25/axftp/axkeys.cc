#include <sodium/core.h>
#include <sodium/crypto_sign.h>
#include <fstream>
#include <iostream>
int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <keybase>\n";
        return 1;
    }
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

    auto fn = std::string(argv[1]) + ".pub";
    std::ofstream pub(fn);
    pub.write(pk, crypto_sign_PUBLICKEYBYTES);
    pub.close();
    if (!pub.good()) {
        throw std::runtime_error("opening " + fn + " failed");
    }

    fn = std::string(argv[1]) + ".priv";
    std::ofstream priv(fn);
    priv.write(sk, crypto_sign_SECRETKEYBYTES);
    priv.close();
    if (!priv.good()) {
        throw std::runtime_error("opening " + fn + " failed");
    }
}
