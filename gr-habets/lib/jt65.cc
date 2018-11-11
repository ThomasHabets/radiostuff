// Requires mz.c from https://physics.princeton.edu/pulsar/k1jt/JT65code.tgz
#include<string>
#include<regex>
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

  //std::cout << "long: " << dlong << std::endl;
  //std::cout << "lat: " << dlat << std::endl;
  //std::cout << "GRID: " << bn << std::endl;
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

// Unpack as callsign.
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
  if (call[0] == ' ') {
    call = call.substr(1,5);
  }
  return call;
}

// Pack callsign.
encoding_t
pack_call(const std::string& callsign)
{
  encoding_t bn=0;

  if (callsign.substr(0,3) == "CQ ") {
    bn += nbase + 1;
    // TODO: handle CQ freq offset.
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

// Pack plaintext.
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

// Unpack plaintext.
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

// Unpack a message into constituent numbers.
std::tuple<encoding_t,encoding_t,encoding_t>
syms2nums(const std::vector<int>& s)
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

std::vector<int>
nums2syms(const encoding_t bn1, const encoding_t bn2, const encoding_t bn3)
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

// Unpack constituent numbers as a CQ.
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
unpack_message(const std::vector<int>& s)
{
  encoding_t nc1, nc2, ng;
  std::tie(nc1, nc2, ng) = syms2nums(s);
  // std::cout  << "Unpack: " << nc1 << " " << nc2 << " " << ng << std::endl;

  // Plaintext.
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
    //std::cout  << "Text unpack: " << nc1 << " " << nc2 << " " << ng << std::endl;
    return unpack_text(nc1, 5) + unpack_text(nc2, 5) + unpack_text(ng, 3);
  }

  // Location call with specific source and destination.
  if (nc1 < encoding_t(nbase)) {
    const auto dst = unpack_call(nc1);
    const auto src = unpack_call(nc2);
    const auto loc = unpack_grid(ng);
    return dst + " " + src + " " + loc;
  }

  // CQ.
  if (nc1 == encoding_t(nbase+1)) {
    const auto cq = unpack_cq(nc1, nc2, ng);
    return "CQ " + cq.first + " " + cq.second;
  }

  // QRZ.
  if (nc1 == nbase + 2) {
    throw std::runtime_error("QRZ not implemented");
  }
  throw std::runtime_error("unknown special type");
}

std::vector<int>
pack_cq(const std::string& src, const std::string& locator)
{
  const auto bn1 = pack_call("CQ ");
  //std::cout << "CQ bn1: " << bn1 << std::endl;

  const auto bn2 = pack_call(" " + src);
  //std::cout << "CQ bn2: " << bn2 << std::endl;

  const auto bn3 = pack_grid("IO91");
  //std::cout << "CQ grid: " << bn3 << std::endl;
  return nums2syms(bn1,bn2,bn3);
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
  if (false) {
    std::cout << "bn1: " << bn1 << std::endl
              << "bn2: " << bn2 << std::endl
              << "bn3: " << bn3 << std::endl;
  }
  return nums2syms(bn1,bn2,bn3);
}

// Pack "DST SRC LOCATOR".
// TODO: Can't encode "DST SRC R-03" etc
std::vector<int>
pack_call(const std::string& dst, const std::string& src, const std::string& locator)
{
  const auto pad = [](const std::string &s){
    return s.size() == 6 ? s : " " + s;
  };
  return nums2syms(pack_call(pad(dst)),
                   pack_call(pad(src)),
                   pack_grid(locator));
}

// Take any message and pack it.
std::vector<int>
pack_message(const std::string& msg)
{
  std::smatch m;

  // CQ
  const std::regex re_cq_loc("CQ (\\w{5,6}) (\\w{4})");
  if (std::regex_match(msg, m, re_cq_loc)) {
    return pack_cq(m[1].str(), m[2].str());
  }

  // Call
  const std::regex re_call_loc("(\\w{5,6}) (\\w{5,6}) (\\w{4})");
  if (std::regex_match(msg, m, re_call_loc)) {
    return pack_call(m[1].str(), m[2].str(), m[3].str());
  }

  // Plaintext
  if (msg.size() <= 13) {
    return pack_plaintext(msg);
  }

  throw std::runtime_error("couldn't encode string of unknown format: \"" + msg + "\", length " + std::to_string(msg.size()));
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

}

/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=2 sw=2
 */
