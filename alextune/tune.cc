#include<errno.h>
#include<fcntl.h>
#include<iostream>
#include<map>
#include<poll.h>
#include<stdexcept>
#include<string.h>
#include<string>
#include<termios.h>
#include<unistd.h>
#include<vector>

void
setup(int fd, int speed)
{
	struct termios tio = {0};
	if (tcgetattr(fd, &tio) != 0) {
		throw std::runtime_error("tcgetattr");
	}

	if (cfsetospeed(&tio, speed)) {
		throw std::runtime_error("cfsetospeed");
	}
	if (cfsetispeed(&tio, speed)) {
		throw std::runtime_error("cfsetispeed");
	}

	// 8 Data bits.
	tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8;

	// No hardware control or parity or other fancy stuff.
	tio.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK);
	tio.c_cflag |= CLOCAL;
	tio.c_cflag &= ~(PARENB | PARODD);
	tio.c_cflag &= ~CRTSCTS;
	tio.c_cflag |= CREAD;

	// 1 Stop bit.
	tio.c_cflag &= ~CSTOPB;
	tio.c_lflag = 0;
	tio.c_oflag = 0;
	tio.c_cc[VMIN]  = 0;
	tio.c_cc[VTIME] = 5;

	if (tcsetattr(fd, TCSANOW, &tio) != 0) {
		throw std::runtime_error("tcsetattr");
	}
}

std::string
command(int fd, const std::string& cmd, bool reply)
{
	//std::clog << "Running command: " << cmd << std::endl;
	{
		const char* p = cmd.data();
		int len = cmd.size();
		for (; len > 0;) {
			const ssize_t n = write(fd, p, len);
			if (n < 0) {
				throw std::runtime_error("write failure");
			}
			len -= n;
			p += n;
		}
		const auto n = write(fd, ";", 1);
		if (n != 1) {
			throw std::runtime_error("write failure");
		}
	}

	if (!reply) {
		return "";
	}
	std::string ret;
	for (;;) {
		char buf[1];
		int n = read(fd, buf, sizeof(buf));
		if (n < 0) {
			throw std::runtime_error("read failure");
		}
		if (n == 0) {
			throw std::runtime_error("EOF");
		}
		ret += std::string(&buf[0], &buf[n]);
		if (buf[n-1] == ';') {
			break;
		}
	}
	//std::clog << "... => " << ret << std::endl;
	return ret.substr(0,ret.size()-1);
}

bool
has_suffix(const std::string& s, const std::string& suffix)
{
	if (suffix.size() > s.size()) {
		return false;
	}
	return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

std::string
trim_prefix(const std::string& s, const std::string& pre)
{
	if (s.substr(0, pre.size()) == pre) {
		return s.substr(pre.size(), s.size());
	}
	return s;
}

std::string
trim_left(const std::string& s)
{
	if (!s.empty() && s[0] == ' ') {
		return trim_left(s.substr(1, s.size()));
	}
	return s;
}

int
run()
{
	std::string portname = "/dev/ttyUSB0";
	int fd = open(portname.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		throw std::runtime_error("open failed");
	}

	setup(fd, B38400);

	// Confirm radio.
	{
		// Confirm protocol.
		const auto id = command(fd, "ID", true);
		if (id != "ID017") {
			throw std::runtime_error("Not protocol 17: " + id);
		}

		// Confirm KX2.
		const auto om = command(fd, "OM", true);
		if (!has_suffix(om, "01")) {
			throw std::runtime_error("Not KX2: " + om);
		}
	}

	// Get power.
	const auto oldPC = command(fd, "PC", true);

	// Turn off ATU and set power to 1W.
	command(fd, "MN023;MP001;SWT09;PC001", false);

	// Confirm power.
	{
		const auto pc = command(fd, "PC", true);
		if (pc != "PC001") {
			throw std::runtime_error("Power not actually set to 1W: %q");
		}
	}

	// Press TUNE.
	command(fd, "SWH16", false);

	std::map<char, char> display_map {
		{	'>',  '-'},
		{		'<',  'l'},
		{		'@',  ' '},
		{		'K',  'H'},
		{		'M',  'N'},
		{		'Q',  'O'},
		{		'V',  'U'},
		{		'W',  'I'},
		{		'X',  '?'}, // c-bar
		{		'Z',  'c'},
		{		'[',  '?'}, // r-bar
		{		'\\', '?'}, // lambda
		{		']',  '?'}, // RX/TX eq level 4
		{		'^',  '?'}, // RX/TX eq level 4
			};

	printf("Tune down SWR and press enter...\n");
	for (;;) {
		struct pollfd fds;
		fds.fd = STDIN_FILENO;
		fds.events = POLLIN;
		if (poll(&fds, 1, 500)) {
			break;
		}
		const auto swr = command(fd, "DS", true);
		std::string out;
		for (int c = 0; c < swr.size(); c++) {
			auto b = swr[c];
			if (b & 128) {
				out.push_back('.');
				b &= 127;
			}
			const auto d = display_map.find(b);
			if (d != display_map.end()) {
				b = d->second;
			}
			out.push_back(b);
		}
		out = trim_prefix(out, "DS");
		out = trim_left(out);
		printf("\r                     \rDS: %s", out.c_str());
		fflush(stdout);
	}
	printf("\n");

	sleep(1);

	// Turn off TUNE.
	command(fd, "RX", false);

	sleep(1);

	// Turn on ATU.
	command(fd, "MN023;MP002;SWT09", false);

	// Set back old power.
	command(fd, oldPC, false);

	// Confirm power.
	{

		const auto pc = command(fd, "PC", true);
		if (pc != oldPC) {
			throw std::runtime_error("Power not actually set back to %q: %q");
		}
	}

	sleep(1);

	// Tune ATU.
	command(fd, "SWT20", false);
}

int
main()
{
	try {
		run();
	} catch(const std::exception&e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
