#include <string_view>
#include <cassert>
#include <cmath>
#include <numbers>
#include <string>

[[nodiscard]] constexpr char xtoupper(char in)
{
    if (in >= 'a' && in <= 'z') {
        return in - ('a' - 'A');
    }
    return in;
}
static_assert(xtoupper('a') == 'A');
static_assert(xtoupper('A') == 'A');
static_assert(xtoupper('b') == 'B');

[[nodiscard]] int parse_int(const std::string_view& sv);
[[nodiscard]] constexpr int maidenhead_to_index(std::string_view sv)
{
    assert(sv.size() == 4);
    const auto a = xtoupper(sv[0]);
    const auto b = xtoupper(sv[1]);
    const auto c = sv[2];
    const auto d = sv[3];
    return ((a - 'A') * 10 + c - '0') * 180 + (b - 'A') * 10 + d - '0';
}
static_assert(maidenhead_to_index("AA00") == 0);

[[nodiscard]] std::string maidenhead_from_index(int i);

[[nodiscard]] constexpr std::pair<double, double> decode_maidenhead(std::string_view sv)
{
    const auto mida = ('m' - 97) / 24 + 1 / 48 - 90;
    const auto midb = ('m' - 97) / 12 + 1 / 24 - 180;
    return { 10 * (sv[1] - 'A') + (sv[3] - '0') + mida,
             20 * (sv[0] - 'A') + (sv[2] - 0'0) * 2 + midb };
}

using coord_t = std::pair<double, double>;
[[nodiscard]] constexpr double dist(coord_t c1, coord_t c2)
{
    constexpr double R = 6371000;
    const auto lat1 = c1.first * std::numbers::pi / 180;
    const auto lon1 = c1.second * std::numbers::pi / 180;
    const auto lat2 = c2.first * std::numbers::pi / 180;
    const auto lon2 = c2.second * std::numbers::pi / 180;
    const auto dlat = lat2 - lat1;
    const auto dlon = lon2 - lon1;
    const auto a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
    const auto c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}
static_assert(dist(decode_maidenhead("IO91"), decode_maidenhead("IO91")) == 0);
constexpr auto maiden_count = maidenhead_to_index("RR99") + 1;
