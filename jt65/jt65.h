// -*- c++ -*-
#include<string>
#include<vector>

namespace JT65 {
#if 0
}
#endif

std::string unpack_message(const std::vector<int>& s);
std::vector<int> pack_message(const std::string& msg);
std::vector<int> greycode(const std::vector<int>& in);
std::vector<int> ungreycode(const std::vector<int>& in);
std::vector<int> interleave(const std::vector<int>& in);
std::vector<int> uninterleave(const std::vector<int>& in);
std::vector<int> fec(const std::vector<int>& in);
std::vector<int> unfec(const std::vector<int>& in);
}
