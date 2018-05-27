#include<iostream>

#include"jt65.h"

using namespace JT65;

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

  const auto p = pack_message("CQ M6VMB IO91");
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
  if (t != in) {
    throw std::runtime_error("decoded to \"" + t + "\", want \"" + in + "\"");
  }
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

  const auto p = pack_message(in);
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
  if (t != in) {
    throw std::runtime_error("decoded to \"" + t + "\", want \"" + in + "\"");
  }
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

  const auto p = pack_message(in);
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
  if (t != in) {
    throw std::runtime_error("decoded to \"" + t + "\", want \"" + in + "\"");
  }
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
/* ---- Emacs Variables ----
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 *
 * vim: ts=2 sw=2
 */
