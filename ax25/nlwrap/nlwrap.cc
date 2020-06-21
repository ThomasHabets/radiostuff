#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct Buf {
  char buf[128];
  size_t size{};
  int read;
  bool eof{false};
  int write;
  Buf(int r, int w): read(r),write(w) {
  }
  int maybe_add(fd_set* rfds, fd_set* wfds) {
    if (size == 0 && !eof) {
      FD_SET(read,rfds);
      return read;
    }
    if (size > 0) {
      FD_SET(write, wfds);
    }
    return write;
  }
  void close() {
    if (size == 0 && write != -1) {
      ::close(write);
      write = -1;
    }
  }
  void maybe_do(fd_set* rfds,fd_set* wfds) {
    if (size == 0 && FD_ISSET(read, rfds)) {
      const auto rc = ::read(read, buf, sizeof(buf));
      if (rc == -1) {
        throw std::runtime_error(std::string("read failed: ") + strerror(errno));
      }
      if (rc == 0) {
        eof = true;
      }
      std::replace(&buf[0], &buf[rc], '\r', '\n');
      size = rc;
      return;
    }
    if (size > 0) {
      // according to manpage a large write may block. how large?
      const auto rc = ::write(write, buf, size);
      if (rc == -1) {
        throw std::runtime_error(std::string("write failed: ") + strerror(errno));
      }
      if (size != rc) {
        std::copy(&buf[rc], &buf[size], &buf[0]);
      }
      size -= rc;
    }
  }
};

static void
usage(const char* av0, int err) {
  printf("Usage: %s: [-h] [-e <stderr file>] [-E <stderr file>] [command]\n", av0);
  exit(err);
}

static int
open_stderr(const char* fn)
{
  if (fn == nullptr) {
    return -1;
  }
  int ret = open(fn, O_APPEND|O_WRONLY|O_CREAT);
  if (ret == -1) {
    throw std::runtime_error(std::string("failed to open stderr file ") + fn + ": " + strerror(errno));
  }
  return ret;
}

static int
mainloop(int pid, int fakeread, int fakewrite)
{
  Buf from_child{fakeread, STDOUT_FILENO};
  Buf from_real{STDIN_FILENO, fakewrite};
  bool child_dead{false};
  for (;;) {
    fd_set rfds, wfds;
    int fd_max = -1;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    FD_ZERO(&wfds);
    FD_ZERO(&rfds);
    fd_max = std::max(from_child.maybe_add(&rfds, &wfds), fd_max);
    fd_max = std::max(from_real.maybe_add(&rfds, &wfds), fd_max);
    // TODO: also handle ^C
    const auto rc = select(fd_max+1, &rfds, &wfds, NULL, &tv);
    if (rc == -1) {
      perror("select");
      continue;
    }
    from_child.maybe_do(&rfds, &wfds);
    from_real.maybe_do(&rfds, &wfds);
    if (from_real.eof && from_child.size == 0) {
      from_real.close();
    }
    if (!child_dead) {
      int status;
      const pid_t p2 = waitpid(pid, &status, WNOHANG);
      if (p2 != 0) {
        if (WIFEXITED(status)) {
          fprintf(stderr, "process exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
          fprintf(stderr, "process killed by signal %d\n", WTERMSIG(status));
        } else {
          fprintf(stderr, "huh, waitpid returned %d but not WIFEXITED? Tracing?\n", p2);
          continue;
        }
        child_dead = true;
      }
    }
    if (child_dead && from_child.size == 0) {
      break;
    }
  }
  return 0;
}

static int
wrapped_main(int argc, char** argv)
{
  const char* child_stderr_filename = nullptr;
  const char* parent_stderr_filename = nullptr;

  {
    int opt;
    while ((opt = getopt(argc, argv, "e:E:h")) != -1) {
      switch (opt) {
      case 'e':
	child_stderr_filename = optarg;
	break;
      case 'E':
	parent_stderr_filename = optarg;
	break;
      case 'h':
	usage(argv[0], EXIT_SUCCESS);
	break;
      default:
	usage(argv[0], EXIT_FAILURE);
	break;
      }
    }
  }
  
  if (optind >= argc) {
    fprintf(stderr, "%s: Expected argument after options\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int child_stderr_file = open_stderr(child_stderr_filename);;
  int parent_stderr_file = open_stderr(parent_stderr_filename);;
  
  int fake0[2];
  int fake1[2];
  if (pipe(fake0)) {
    fprintf(stderr, "%s: failed to create first pipe: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }
  if (pipe(fake1)) {
    fprintf(stderr, "%s: failed to create second pipe: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }

  const pid_t pid = fork();
  switch (pid) {
  case -1:
    fprintf(stderr, "%s: failed to fork: %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  case 0:
    close(fake0[1]);
    close(fake1[0]);
    if (-1 == dup2(fake0[0], STDIN_FILENO)) {
      throw std::runtime_error(std::string("dup2 into stdin: ") + strerror(errno));
    }
    if (-1 == dup2(fake1[1], STDOUT_FILENO)) {
      throw std::runtime_error(std::string("dup2 into stdout: ") + strerror(errno));
    }
    close(fake0[0]);
    close(fake1[1]);
    if (child_stderr_file != -1) {
      if (-1 == dup2(child_stderr_file, STDERR_FILENO)) {
	throw std::runtime_error(std::string("dup2 into stderr: ") + strerror(errno));
      }
    }
    close(child_stderr_file);
    execvp(argv[optind], &argv[optind]);
    fprintf(stderr, "%s: execv of %s: %s\n", argv[0], argv[optind], strerror(errno));
    exit(EXIT_FAILURE);
  }
  try {
    close(fake0[0]);
    close(fake1[1]);
    return mainloop(pid, fake1[0], fake0[1]);
  } catch (...) {
    if (-1 == kill(pid, SIGTERM)) {
      fprintf(stderr, "%s: Failed to kill child process during exception: %s\n", argv[0], strerror(errno));
    }
    throw;
  }
}

int main(int argc, char** argv) {
  try {
    return wrapped_main(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << argv[0] << ": exited due to exception: " << e.what() << std::endl;
  } catch (...) {
    std::cerr << argv[0] << ": exited due to unknown exception\n";
  }
  return EXIT_FAILURE;
}
