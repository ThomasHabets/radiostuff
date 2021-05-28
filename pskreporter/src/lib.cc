#include "lib.h"
#include <cstdlib>
#include <stdexcept>

[[nodiscard]] std::string maidenhead_from_index(int i)
{
    std::string ret = "AA00";
    const auto x = i / 180;
    const auto y = i % 180;
    ret[0] = 'A' + (x / 10);
    ret[1] = 'A' + (y / 10);
    ret[2] = '0' + (x % 10);
    ret[3] = '0' + (y % 10);
    return ret;
}

#if 0
[[nodiscard]] int parse_int(std::string_view sv)
{
#if 0
    char* endptr = nullptr;
    const auto ret = strtol(sv.data(), &endptr, 10);
    if (endptr != sv.end()) {
        throw std::runtime_error(std::string("int parsing of ") + std::string(sv) +
                                 " failed");
    }
    return ret;
#else
    int ret = 0;
    for (const auto& ch : sv) {
      if (ch < '0' || ch > '9') {
        throw std::runtime_error(std::string("int parsing of ") + std::string(sv) +
                                 " failed");
      }
      ret = 10*ret + ch - '0';
    }
    return ret;
#endif
}
#endif
