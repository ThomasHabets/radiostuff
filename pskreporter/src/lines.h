#include <string_view>
#include <array>
#include <iostream>
#include <string>

class Lines
{
public:
    Lines(std::string_view data) : data_(std::move(data)) {}

    class Iterator
    {
    public:
        Iterator(std::string_view sv, bool end = false) : sv_(std::move(sv)), end_(end)
        {
            if (!end_) {
                read();
            }
        }

        [[nodiscard]] bool operator!=(const Iterator& rhs) const
        {
            return !(end_ && rhs.end_);
        }

        Iterator& operator++()
        {
            read();
            return *this;
        }

        [[nodiscard]] const std::string_view& operator*() const { return cur_; }

    private:
        void read()
        {
            if (end_) {
                return;
            }
            auto nl = sv_.find('\n');
            if (nl == std::string_view::npos) {
                end_ = true;
                return;
            }
            cur_ = { sv_.begin(), nl };
            sv_ = { sv_.begin() + nl + 1, sv_.end() };
        }

        std::string_view cur_;
        std::string_view sv_;
        bool end_;
    };

    [[nodiscard]] Iterator begin() const { return Iterator{ data_ }; }

    [[nodiscard]] Iterator end() const { return Iterator("", true); }

private:
    const std::string_view data_;
};

class StreamLines
{
public:
    StreamLines(std::istream& stream) : stream_(stream) {}

    class Iterator
    {
    public:
        Iterator(std::istream* stream, bool end = false) : stream_(stream), end_(end)
        {
            if (!end_) {
                read();
            }
        }

        [[nodiscard]] bool operator!=(const Iterator& rhs) const
        {
            return !(end_ && rhs.end_);
        }

        Iterator& operator++()
        {
            read();
            return *this;
        }

        [[nodiscard]] const std::string_view operator*() const { return cur_; }

    private:
        void read()
        {
            if (end_) {
                return;
            }
            cur_ = "";
            getline(*stream_, cur_);
            if (cur_.empty()) {
                end_ = true;
                return;
            }
            const auto e = cur_.find_last_not_of("\n");
            cur_ = (e == std::string::npos) ? "" : cur_.substr(0, e + 1);
        }
        std::istream* stream_;
        std::string cur_;
        bool end_;
    };

    [[nodiscard]] Iterator begin() const { return Iterator{ &stream_ }; }

    [[nodiscard]] Iterator end() const { return Iterator(nullptr, true); }

private:
    std::istream& stream_;
};

template <int Size>
[[nodiscard]] std::array<std::string_view, Size> split(std::string_view sv)
{
    const auto orig = sv;
    std::array<std::string_view, Size> ret;
    for (size_t c = 0; c < ret.size() - 1; c++) {
        auto n = sv.find(',');
        if (n == std::string_view::npos) {
            // throw std::runtime_error("splitting <" + std::string(orig) + "> into " +
            // std::to_string(Size) + " failed");
            for (auto& o : ret) {
                o = { orig.begin(), orig.begin() };
            }
            return ret;
        }
        ret[c] = { sv.begin(), n };
        sv = { sv.begin() + n + 1, sv.end() };
    }
    ret[ret.size() - 1] = { sv.begin(), sv.end() };
    return ret;
}
