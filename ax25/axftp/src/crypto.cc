#include "axlib.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>

#include <sodium/core.h>
#include <sodium/crypto_sign.h>

#include <string>
namespace axlib {

namespace crypto {
namespace {
bool inited = false;
void init()
{
    if (!inited) {
        if (sodium_init() == -1) {
            throw std::runtime_error("sodium_init() failed");
        }
        inited = true;
    }
}

} // namespace

bool verify(const std::string& data, const std::array<char, 32>& pk)
{
    init();
    const auto sig = data.substr(0, 64);
    const auto msg = data.substr(64);
    if (crypto_sign_verify_detached(reinterpret_cast<const unsigned char*>(sig.data()),
                                    reinterpret_cast<const unsigned char*>(msg.data()),
                                    msg.size(),
                                    reinterpret_cast<const unsigned char*>(pk.data())) !=
        0) {
        return false;
    }
    return true;
}

std::string sign(const std::string& msg, const std::array<char, 64>& sk)
{
    init();

    unsigned char sig[crypto_sign_BYTES];

    crypto_sign_detached(sig,
                         NULL,
                         reinterpret_cast<const unsigned char*>(msg.data()),
                         msg.size(),
                         reinterpret_cast<const unsigned char*>(sk.data()));
    return std::string(&sig[0], &sig[crypto_sign_BYTES]);
}

} // namespace crypto
} // namespace axlib
