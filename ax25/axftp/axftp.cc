#include <functional>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include "axlib.h"

using namespace axlib;

namespace {
void usage(const char *av0, int err) {
  FILE *f = stdout;
  if (err) {
    f = stderr;
  }
  fprintf(
      f,
      "Usage: %s [-h] [-e] [-w <window>] -s <src call> [-p <digipath>] <dst>\n",
      av0);
  exit(err);
}
} // namespace

int main(int argc, char **argv) {
  CommonOpts copt;
  int opt;
  while ((opt = getopt(argc, argv, "ehl:p:s:w:")) != -1) {
    if (common_opt(copt, opt)) {
      continue;
    }
    switch (opt) {
    case 'h':
      usage(argv[0], EXIT_SUCCESS);
    default: /* '?' */
      usage(argv[0], EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    fprintf(stderr, "Need dest\n");
    exit(EXIT_FAILURE);
  }
  const std::string dst = argv[optind];

  SeqPacket sock{copt.src, copt.path};
  sock.set_window_size(copt.window);
  sock.set_packet_length(copt.packet_length);
  sock.set_extended_modulus(copt.extended_modulus);

  std::clog << "Connecting...\n";
  if (sock.connect(dst)) {
    std::clog << "Failed to connect!\n";
    return 1;
  }
  std::clog << "Connected!\n";
  const auto total = atoi(sock.read().c_str());
  std::clog << "Total size: " << total << std::endl;
  int rcvd = 0;
  for (;;) {
    const auto s = sock.read();
    std::clog << "Got data of size " << s.size() << std::endl;
    if (s.empty()) {
      break;
    }
    rcvd += s.size();
    std::cout << s << std::flush;
    if (total == rcvd) {
      break;
    }
  }
  sock.write("OK");
  return 0;
}
