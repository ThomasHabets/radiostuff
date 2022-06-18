#include "selectloop.h"

#include <sys/select.h>
#include <system_error>
#include <cerrno>
#include <cstddef>

int selectloop(Ingress& side1, Ingress& side2)
{
    for (;;) {
        fd_set rfds;
        FD_ZERO(&rfds);

        // Always check for data to read.
        FD_SET(side1.fd(), &rfds);
        FD_SET(side2.fd(), &rfds);
        int max = std::max(side1.fd(), side2.fd());

        // Conditionally wait for ready to write.
        fd_set wfds;
        FD_ZERO(&wfds);
        if (!side1.out().empty()) {
            const auto t = side1.out().fd();
            FD_SET(t, &wfds);
            max = std::max(max, t);
        }
        if (!side2.out().empty()) {
            const auto t = side2.out().fd();
            FD_SET(t, &wfds);
            max = std::max(max, t);
        }

        const auto rc = select(max + 1, &rfds, &wfds, NULL, NULL);
        if (rc < 0) {
            throw std::system_error(errno, std::generic_category(), "select()");
        }

        if (rc == 0) {
            // Shouldn't happen.
            continue;
        }

        if (FD_ISSET(side1.fd(), &rfds)) {
            if (!side1.read()) {
                return EXIT_FAILURE;
            }
        }

        if (FD_ISSET(side2.fd(), &rfds)) {
            if (!side2.read()) {
                return EXIT_FAILURE;
            }
        }

        if (FD_ISSET(side1.out().fd(), &wfds)) {
            side1.out().send();
        }
        if (FD_ISSET(side2.out().fd(), &wfds)) {
            side2.out().send();
        }
    }
}
