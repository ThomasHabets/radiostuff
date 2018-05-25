// Requires mz.c from https://physics.princeton.edu/pulsar/k1jt/JT65code.tgz
#include<string>
#include<tuple>
#include<iostream>
#include<vector>
#include<stdexcept>

extern void rs_init_();
extern void rs_encode_(int*,int*);
extern void rs_decode_(int *recd0, int *era0, int *numera0, int *decoded);

namespace JT65 {
#if 0
}
#endif

constexpr uint64_t nbase = 37*36*10*27*27*27;
const std::string alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ +-./?";
typedef uint64_t encoding_t;

// Return the index of the character.
int
nchar(char ch)
{
  for (int i = 0; i < alphabet.size(); i++) {
    if (ch == alphabet[i]) {
      return i;
    }
  }
  throw std::runtime_error("invalid character supplied: " + std::to_string(ch));
}

// Pack grid coordinates into a encoding_t.
encoding_t
pack_grid(const std::string& grid)
{
  const int lng = 180 - 20*(grid[0]-'A') - 2*(grid[2]-'0');
  const int lat = -90+10*(grid[1]-'A') + grid[3]-'0';

  // TODO: interpret 4-char as ending in 'mm', and parse 6-char ones.
  const float xminlong = 5*('m'-'a'+0.5);
  const float xminlat = 2.5*('m'-'a'+0.5);
  const float dlong = lng - xminlong/60.0;
  const float dlat = lat + xminlat/60.0;
  const float ng = ((int(dlong)+180)/2)*180+int(dlat+90);

  const encoding_t bn{static_cast<unsigned long>(ng)};

  std::cout << "long: " << dlong << std::endl;
  std::cout << "lat: " << dlat << std::endl;
  std::cout << "GRID: " << bn << std::endl;
  return bn;
}

// TODO: handle 6-character grid locators.
std::string
unpack_grid(const encoding_t n)
{
  auto dlat = (n % 180) - 90;
  auto dlong = (n / 180)*2 - 180 + 2;
  if (dlong < -180) {
    dlong += 360;
  }
  if (dlong > 180) {
    dlong -= 360;
  }
  const float nlong = 60.0*(180.0-dlong)/5.0;
  const float nlat = 60.0*(dlat+90)/2.5;
  std::string ret = "....";

  int n1 = nlong / 240.0;
  int n2 = (nlong-240*n1)/24;
  int n3 = nlong-240*n1-24*n2;
  ret[0] = 'A' + n1;
  ret[2] = '0' + n2;
  //ret[4] = 'a' + n3;

  n1 = nlat / 240.0;
  n2 = (nlat-240*n1)/24;
  n3 = nlat-240*n1-24*n2;
  ret[1] = 'A' + n1;
  ret[3] = '0' + n2;
  //ret[5] = 'a' + n3;
  return ret;
}

std::string
unpack_call(encoding_t n)
{
  if (n > nbase) {
    std::cout << "unpack_call: plaintext\n";
  }
  std::string call = "......";
  const std::string& a = alphabet;
  call[5] = a[10+n%27];
  n /= 27;
  call[4] = a[10+n%27];
  n /= 27;
  call[3] = a[10+n%27];
  n /= 27;
  call[2] = a[n%10];
  n /= 10;
  call[1] = a[n%36];
  n /= 36;
  call[0] = a[n];
  return call;
}

encoding_t
pack_call(const std::string& callsign)
{
  encoding_t bn=0;

  if (callsign.substr(0,3) == "CQ ") {
    bn += nbase + 1;
    // TODO: handle CQ freq offset.
    std::cout << "Yeah, CQ\n";
    return bn;
  }

  bn += nchar(callsign[0]);
  bn *= 36; bn += nchar(callsign[1]);
  bn *= 10; bn += nchar(callsign[2]);
  bn *= 27; bn += nchar(callsign[3])-10;
  bn *= 27; bn += nchar(callsign[4])-10;
  bn *= 27; bn += nchar(callsign[5])-10;

  return bn;
}

encoding_t
pack_text(const std::string& txt)
{
  if (txt.size() > 5) {
    throw std::runtime_error("can't pack text longer than 5-chars");
  }
  encoding_t bn = 0;

  for (const auto& ch : txt) {
    bn = bn * 42 + nchar(ch);
  }
  return bn;
}

std::string
unpack_text(encoding_t in, int chars)
{
  std::string ret(chars, ' ');
  for (int i = chars-1; i >= 0; --i) {
    ret[i] = alphabet[in % alphabet.size()];
    in /= alphabet.size();
  }
  return ret;
}

std::tuple<encoding_t,encoding_t,encoding_t>
message_to_nums(const std::vector<int>& s)
{
  const encoding_t nc1 =
    (encoding_t(s[0]) << 22)
    + (encoding_t(s[1]) << 16)
    + (encoding_t(s[2]) << 10)
    + (encoding_t(s[3]) << 4)
    + ((encoding_t(s[4]) >> 2) % 16);

  const encoding_t nc2 =
    (encoding_t(s[4] & 3) << 26)
    + (encoding_t(s[5]) << 20)
    + (encoding_t(s[6]) << 14)
    + (encoding_t(s[7]) << 8)
    + (encoding_t(s[8]) << 2)
    + ((encoding_t(s[9]) >> 4) % 16);

  const encoding_t ng =
    (encoding_t(s[9] & 15) << 12)
    + (encoding_t(s[10]) << 6)
    + encoding_t(s[11]);
  return std::make_tuple(nc1, nc2, ng);
}

std::pair<std::string, std::string>
unpack_cq(const encoding_t nc1, const encoding_t nc2, const encoding_t ng)
{
  const auto nfreq = nc1 - nbase - 3;
  if (nfreq > 0 && nfreq < 1000) {
    std::cout << "Decode: nfreq is " << nfreq << std::endl;
  }
  std::string callsign;
  if (nc2 < nbase) {
    callsign = unpack_call(nc2);
  }
  std::string loc = unpack_grid(ng);

  return std::make_pair(callsign, loc);
}

// TODO: return structured data.
std::string
unpack_message(std::vector<int>& s)
{
  encoding_t nc1, nc2, ng;
  std::tie(nc1, nc2, ng) = message_to_nums(s);
  std::cout  << "Unpack: " << nc1 << " " << nc2 << " " << ng << std::endl;

  if (ng > 32768) {
    ng &= 32767;
    if (nc1 & 1) {
      ng += 32768;
    }
    nc1 /= 2;
    if (nc2 & 1) {
      ng += 65536;
    }
    nc2 /= 2;
    std::cout  << "Text unpack: " << nc1 << " " << nc2 << " " << ng << std::endl;
    return unpack_text(nc1, 5) + unpack_text(nc2, 5) + unpack_text(ng, 3);
  }

  if (nc1 < encoding_t(nbase)) {
    const auto dst = unpack_call(nc1);
    const auto src = unpack_call(nc2);
    const auto loc = unpack_grid(ng);
    return dst + " " + src + " " + loc;
  }

  if (nc1 == encoding_t(nbase+1)) {
    const auto cq = unpack_cq(nc1, nc2, ng);
    return "CQ " + cq.first + " " + cq.second;
  }

  if (nc1 == nbase + 2) {
    throw std::runtime_error("QRZ not implemented");
  }
  throw std::runtime_error("unknown special type");
}

std::vector<int>
pack_numbers(const encoding_t bn1, const encoding_t bn2, const encoding_t bn3)
{
  return std::vector<int>{
    static_cast<int>((bn1 >> 22) % 64),
      static_cast<int>((bn1 >> 16) % 64),
      static_cast<int>((bn1 >> 10) % 64),
      static_cast<int>((bn1 >> 4) % 64),
      static_cast<int>(4*(bn1 & 15) + ((bn2 >> 26) & 3)),
      static_cast<int>((bn2 >> 20) % 64),
      static_cast<int>((bn2 >> 14) % 64),
      static_cast<int>((bn2 >> 8) % 64),
      static_cast<int>((bn2 >> 2) % 64),
      static_cast<int>(16*(bn2 & 3) + ((bn3 >> 12) & 15)),
      static_cast<int>((bn3 >> 6) % 64),
      static_cast<int>(bn3 & 63),
  };
}

std::vector<int>
pack_cq(const std::string& src, const std::string& locator)
{
  const auto bn1 = pack_call("CQ ");
  std::cout << "CQ bn1: " << bn1 << std::endl;

  const auto bn2 = pack_call(" " + src);
  std::cout << "CQ bn2: " << bn2 << std::endl;

  const auto bn3 = pack_grid("IO91");
  std::cout << "CQ grid: " << bn3 << std::endl;
  return pack_numbers(bn1,bn2,bn3);
}

std::vector<int>
pack_plaintext(const std::string& msg)
{
  auto bn1 = pack_text(msg.substr(0, 5));
  bn1 *= 2;

  auto bn2 = pack_text(msg.substr(5, 5));
  bn2 *= 2;

  auto bn3 = pack_text(msg.substr(10, 3));
  if (bn3 & 32768) {
    bn1++;
  }
  if (bn3 & 65536) {
    bn2++;
  }
  bn3 &= 32767;
  bn3 += 32768;
  std::cout << "bn1: " << bn1 << std::endl
            << "bn2: " << bn2 << std::endl
            << "bn3: " << bn3 << std::endl;
  return pack_numbers(bn1,bn2,bn3);
}

// Pack "DST SRC LOCATOR".
// TODO: Can't encode "DST SRC R-03" etc
std::vector<int>
pack_call(const std::string& dst, const std::string& src, const std::string& locator)
{
  const auto pad = [](const std::string &s){
    return s.size() == 6 ? s : " " + s;
  };
  return pack_numbers(pack_call(pad(dst)),
                      pack_call(pad(src)),
                      pack_grid(locator));
}

std::vector<int>
greycode(const std::vector<int>& in)
{
  std::vector<int> ret;
  for (const auto& t : in) {
    ret.push_back(t ^ t/2);
  }
  return ret;
}

std::vector<int>
ungreycode(const std::vector<int>& in)
{
  // TODO: ugly solution.
  std::vector<int> ret;
  for (const auto& t : in) {
    for (int i = 0; i < 100; i++) {
      if ((i ^ i/2) == t) {
        ret.push_back(i);
        break;
      }
    }
  }
  return ret;
}

std::vector<int>
interleave(const std::vector<int>& in)
{
  std::vector<int> ret;
  for (int j = 0; j < 7; j++) {
    for (int i = 0; i < 9; i++) {
      ret.push_back(in[i * 7 + j]);
    }
  }
  return ret;
}

std::vector<int>
uninterleave(const std::vector<int>& in)
{
  std::vector<int> ret;
  for (int i = 0; i < 9; i++) {
    for (int j = 0; j < 7; j++) {
      ret.push_back(in[j * 9 + i]);
    }
  }
  return ret;
}

std::vector<int>
fec(const std::vector<int>& in)
{
  std::vector<int> ret(63);
  rs_init_();
  rs_encode_(const_cast<int*>(&in[0]), &ret[0]);
  return ret;
}

std::vector<int>
unfec(const std::vector<int>& in)
{
  int era = 0;
  std::vector<int> ret(63);
  rs_init_();
  rs_decode_(const_cast<int*>(&in[0]), NULL, &era, &ret[0]);
  ret.resize(12);
  return ret;
}

// Check that output from a stage is equal to the expected, and
// dump both expected and actual if they don't match.
void
test_step(const std::string& name,
          const std::vector<int>& got,
          const std::vector<int>& want)
{
  if (got != want) {
    std::cout << "Error at " << name << std::endl;
    std::cout << "Got:\n  ";
    for (int i = 0; i < got.size(); i++) {
      if (i % 8 == 0) {
        std::cout  << "\n  ";
      }
      printf("%2d ", int(got[i]));
    }
    std::cout << std::endl;
    std::cout << "Want:\n  ";
    for (int i = 0; i < want.size(); i++) {
      if (i % 8 == 0) {
        std::cout  << "\n  ";
      }
      printf("%2d ", int(want[i]));
    }
    std::cout << std::endl;
    throw std::runtime_error("wrong output");
  }
}
}

using namespace JT65;

void
test_cq()
{
  std::cout << "Testing CQ\n";
  const std::string in = "CQ M6VMB IO91";
  const std::vector<int> packed{62, 32, 32, 49, 39, 55, 34, 1, 22, 3, 63, 21};
  const std::vector<int> fecced{
    56, 1, 31, 43, 49, 61, 18, 38,
      63, 37, 58, 44, 58, 62, 45, 43,
      17, 22, 60, 2, 20, 8, 27, 26,
      48, 41, 37, 7, 41, 0, 60, 25,
      56, 62, 46, 15, 39, 43, 27, 10,
      12, 2, 51, 5, 53, 0, 0, 58,
      23, 37, 7, 62, 32, 32, 49, 39,
      55, 34, 1, 22, 3, 63, 21,
  };
  const std::vector<int> interleaved{
    56, 38, 45, 8, 41, 15, 51, 37, 55, 1, 63, 43, 27, 0, 39, 5, 7, 34, 31,
      37, 17, 26, 60, 43, 53, 62, 1, 43, 58, 22, 48, 25, 27, 0, 32, 22, 49,
      44, 60, 41, 56, 10, 0, 32, 3, 61, 58, 2, 37, 62, 12, 58, 49, 63, 18,
      62, 20, 7, 46, 2, 23, 39, 21,
  };

  const std::vector<int> greyed{
    // After greycode:
    36, 53, 59, 12, 61, 8, 42, 55, 44, 1, 32, 62, 22, 0, 52, 7, 4, 51, 16, 55, 25,
    23, 34, 62, 47, 33, 1, 62, 39, 29, 40, 21, 22, 0, 48, 29, 41, 58, 34, 61, 36, 15,
    0, 48, 2, 35, 39, 3, 55, 33, 10, 39, 41, 32, 27, 33, 30, 4, 57, 3, 28, 52, 31,
  };

  const auto p = pack_cq("M6VMB", "IO91");
  test_step("pack", p, packed);

  // Test fecced.
  auto f = fec(p);
  test_step("fec", f, fecced);

  f = interleave(f);
  test_step("interleave", f, interleaved);

  f = greycode(f);
  test_step("greycode", f, greyed);

  f = ungreycode(f);
  test_step("ungreycode", f, interleaved);

  f = uninterleave(f);
  test_step("uninterleave", f, fecced);

  f = unfec(f);
  test_step("unfec", f, packed);

  auto t = unpack_message(f);
  std::cout << "Decoded: " << t << std::endl;
}

void
test_call()
{
  std::cout << "Testing call\n";
  const std::string in = "N4CCB M6VMB IO91";
  const std::vector<int> packed{61, 58, 44, 20, 31, 55, 34, 1, 22, 3, 63, 21};
  const std::vector<int> fecced{
    26, 30, 2, 55, 45, 20, 10, 23, 35, 30, 12, 23, 0, 18, 57, 21, 15, 31, 33,
      31, 32, 8, 44, 20, 1, 62, 27, 5, 18, 30, 38, 30, 19, 25, 6, 6, 40, 41,
      44, 18, 62, 26, 59, 62, 22, 47, 18, 59, 4, 14, 63, 61, 58, 44, 20, 31,
      55, 34, 1, 22, 3, 63, 21,
  };
  const std::vector<int> interleaved{
    26, 23, 57, 8, 18, 6, 59, 14, 55, 30, 35, 21, 44, 30, 40, 62, 63, 34,
      2, 30, 15, 20, 38, 41, 22, 61, 1, 55, 12, 31, 1, 30, 44, 47, 58, 22,
      45, 23, 33, 62, 19, 18, 18, 44, 3, 20, 0, 31, 27, 25, 62, 59, 20,
      63, 10, 18, 32, 5, 6, 26, 4, 31, 21,
      };

  const std::vector<int> greyed{
    23, 28, 37, 12, 27, 5, 38, 9, 44, 17, 50, 31, 58, 17, 60, 33, 32, 51, 3,
      17, 8, 30, 53, 61, 29, 35, 1, 44, 10, 16, 1, 17, 58, 56, 39, 29, 59,
      28, 49, 33, 26, 27, 27, 58, 2, 30, 0, 16, 22, 21, 33, 38, 30, 32, 15,
      27, 48, 7, 5, 23, 6, 16, 31,
  };

  const auto p = pack_call("N4CCB", "M6VMB", "IO91");
  test_step("pack", p, packed);

  // Test fecced.
  auto f = fec(p);
  test_step("fec", f, fecced);

  f = interleave(f);
  test_step("interleave", f, interleaved);

  f = greycode(f);
  test_step("greycode", f, greyed);

  f = ungreycode(f);
  test_step("ungreycode", f, interleaved);

  f = uninterleave(f);
  test_step("uninterleave", f, fecced);

  f = unfec(f);
  test_step("unfec", f, packed);

  auto t = unpack_message(f);
  std::cout << "Decoded: " << t << std::endl;
}

void
test_plaintext()
{
  std::cout << "Testing plaintext\n";
  const std::string in = "1234567890123";
  const std::vector<int> packed{1, 35, 41, 39, 8, 36, 40, 9, 41, 8, 28, 59};
  const std::vector<int> fecced{
    9, 12, 20, 18, 58, 54, 9, 20, 53, 59, 63, 19, 20, 39, 4, 58, 15, 7, 26, 51, 42, 57, 11, 58, 58, 26, 39, 47, 2, 41, 0, 33, 61, 55, 49, 31, 24, 60, 24, 19, 60, 27, 20, 20, 21, 2, 56, 8, 54, 60, 17, 1, 35, 41, 39, 8, 36, 40, 9, 41, 8, 28, 59,
  };
  const std::vector<int> interleaved{
    9, 20, 4, 57, 2, 31, 20, 60, 36, 12, 53, 58, 11, 41, 24, 20, 17, 40, 20, 59, 15, 58, 0, 60, 21, 1, 9, 18, 63, 7, 58, 33, 24, 2, 35, 41, 58, 19, 26, 26, 61, 19, 56, 41, 8, 54, 20, 51, 39, 55, 60, 8, 39, 28, 9, 39, 42, 47, 49, 27, 54, 8, 59, 
  };

  const std::vector<int> greyed{
    13, 30, 6, 37, 3, 16, 30, 34, 54, 10, 47, 39, 14, 61, 20, 30, 25, 60, 30, 38, 8, 39, 0, 34, 31, 1, 13, 27, 32, 4, 39, 49, 20, 3, 50, 61, 39, 26, 23, 23, 35, 26, 36, 61, 12, 45, 30, 42, 52, 44, 34, 12, 52, 18, 13, 52, 63, 56, 41, 22, 45, 12, 38,
  };

  const auto p = pack_plaintext(in);
  test_step("pack", p, packed);

  // Test fecced.
  auto f = fec(p);
  test_step("fec", f, fecced);

  f = interleave(f);
  test_step("interleave", f, interleaved);

  f = greycode(f);
  test_step("greycode", f, greyed);

  f = ungreycode(f);
  test_step("ungreycode", f, interleaved);

  f = uninterleave(f);
  test_step("uninterleave", f, fecced);

  f = unfec(f);
  test_step("unfec", f, packed);

  auto t = unpack_message(f);
  std::cout << "Decoded: " << t << std::endl;
}


int
main(int argc, char** argv)
{
  try {
    test_cq();
    std::cout << "----------------\n";
    test_call();
    std::cout << "----------------\n";
    test_plaintext();
  } catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
