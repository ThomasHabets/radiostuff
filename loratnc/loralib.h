#include <termios.h>
#include <string>
#include <vector>

class LoStik
{
public:
    LoStik(std::string dev);
    std::string cmd(const std::string&);
    std::string readline();
    int get_fd() const noexcept { return fd_; }

    bool send(const std::vector<unsigned char>&);
    std::vector<unsigned char> process();
    bool sending_in_progress() const { return send_in_progress_; }

private:
    void flush();
    const std::string dev_;
    int fd_ = -1;
    bool send_in_progress_ = false;

    static constexpr int speed = B57600;

    // TODO: create a send() that takes pending_tx into account.
    bool pending_tx_ = false;
};
