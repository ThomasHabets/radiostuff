#include "axlib.h"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>

using namespace axlib;

namespace {

void handle2(SeqPacket conn) {
  FILE *f = fopen("test.txt", "r");
  if (f == nullptr) {
    throw std::runtime_error(std::string("opening test.txt: ") +
                             strerror(errno));
  }
  fseek(f, 0, SEEK_END);
  const auto size = ftell(f);
  fseek(f, 0, SEEK_SET);
  conn.write(std::to_string(size));
  const auto len = conn.packet_length();
  std::clog << "Handling connection with packet length " << len << std::endl;
  for (;;) {
    std::vector<char> buf(len);
    const auto n = fread(&buf[0], 1, buf.size(), f);
    if (n == 0) {
      break;
    }
    if (n == -1) {
      throw std::runtime_error(std::string("reading stdin: ") +
                               strerror(errno));
    }
    std::clog << "Sending packet of size " << n << std::endl;
    conn.write(std::string(&buf[0], &buf[n]));
  }
  std::clog << "reply: " << conn.read() << std::endl;
  ;
  std::clog << "closing connection\n";
}

void handle(SeqPacket conn) {
  try {
    handle2(std::move(conn));
  } catch (const std::exception &e) {
    std::cerr << "Exception in client handler: " << e.what() << std::endl;
  }
}

void usage(const char *av0, int err) {
  FILE *f = stdout;
  if (err) {
    f = stderr;
  }
  fprintf(f,
          "Usage: %s [-h] [-e] [-w <window> ] -s <my call> [-p <digipath>]\n",
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

  if (optind != argc) {
    fprintf(stderr, "Extra args on command line\n");
    exit(EXIT_FAILURE);
  }

  SeqPacket sock{copt.src, copt.path};
  sock.set_window_size(copt.window);
  sock.set_packet_length(copt.packet_length);
  sock.set_extended_modulus(copt.extended_modulus);

  std::clog << "Listening...\n";
  sock.listen([](SeqPacket conn) {
    std::clog << "Connection from " << conn.peer_addr() << "\n";
    std::thread th(handle, std::move(conn));
    th.detach();
  });

  return 0;
}
