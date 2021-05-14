#include <string_view>
#include <functional>
#include<memory>
#include<mutex>
#include<condition_variable>
#include<thread>
#include<cassert>
#include <unistd.h>
#include <array>
#include<sys/mman.h>
#include <iostream>
#include <cstring>
#include <string>

class Lines {
public:
  using read_t = std::function<std::pair<std::string_view,bool>()>;
  class Iterator {
  public:
    Iterator(read_t &&read, bool end = false) : read_(std::move(read)), end_(end)
        {
            if (!end_) {
                read_();
            }
        }
        [[nodiscard]] bool operator!=(const Iterator& rhs) const
        {
            return !(end_ && rhs.end_);
        }
        Iterator& operator++()
        {
            if (!end_) {
	      auto r = read_();
	      cur_ = r.first;
	      end_ = r.second;
	    }
            return *this;
        }
        [[nodiscard]] const std::string_view& operator*() const { return cur_; }

    // No copy.
    Iterator(const Iterator&) = delete;
    Iterator& operator=(const Iterator&) = delete;
  private:
    const read_t read_;
    std::string_view cur_;
    bool end_;
  };
  
  [[nodiscard]] virtual Iterator begin() = 0;
  [[nodiscard]] Iterator end() const  {    return Iterator(read_t{}, true);  }
};

class MemLines: public Lines
{
public:
  MemLines(std::string_view data) : data_(std::move(data)) {}

  [[nodiscard]] Lines::Iterator begin() override
  {
    return Lines::Iterator( [sv = data_]()mutable ->auto   {
            auto nl = sv.find('\n');
            if (nl == std::string_view::npos) {
	      return std::make_pair(std::string_view{}, true);
            }
	    std::string_view cur{ sv.begin(), nl };
            sv = { sv.begin() + nl + 1, sv.end() };
	    return std::pair<std::string_view, bool>(cur, false);
    });
  }

private:
  const std::string_view data_;
};

class StreamLines: public Lines
{
public:
  StreamLines(std::istream& stream) : stream_(stream) {}
  [[nodiscard]] Lines::Iterator begin() override
  {
    return Lines::Iterator( [this, cur = std::string{}]()mutable ->auto   {
			      getline(stream_, cur);
			      if (stream_.eof()) {
				return std::make_pair(std::string_view{}, true);
			      }
			      if (!stream_.good()) {
				throw std::runtime_error(std::string("failed to read line: ") + strerror(errno));
			      }
			      const auto e = cur.find_last_not_of("\n");
			      cur = (e == std::string::npos) ? "" : cur.substr(0, e + 1);
			      return std::make_pair(std::string_view{cur}, false);
			    });
  }
private:
    std::istream& stream_;
};

class StreamManualLines: public Lines
{
public:
  StreamManualLines(std::istream& stream) : stream_(stream) {}
  [[nodiscard]] Lines::Iterator begin() override
  {
    // Experimentally 8KiB seems to give best
    // performance for the other method.
    constexpr int chunk_size = 8192;
    return Lines::Iterator( [this, buf = std::vector<char>(), pos = size_t{0}]()mutable ->auto   {
			      auto start = buf.data() + pos;
			      auto end = buf.data() + buf.size();
			      auto e = std::find(start, end, '\n');
			      if (e == end) {
				// small copy; less than a line.
				assert(end-start<100);
				std::copy(start, end, std::begin(buf));
				const auto old_size = end-start;
				start = buf.data();
				buf.resize(old_size+chunk_size);
				stream_.read(&buf[old_size], chunk_size);
				const auto n = stream_.gcount();
				if (chunk_size != n) {
				  buf.resize(old_size+n);
				}
				pos = 0;
				start = buf.data();
				end = buf.data() + buf.size();
				e = std::find(start, end, '\n');
				if (e == end) {
				  // TODO: check eofbit
				  return std::make_pair(std::string_view{}, true);
				}
			      }
			      pos += e - start + 1;
			      return std::make_pair(std::string_view{start, e}, false);
			    });

  }
private:
  std::istream& stream_;
};

class StreamMapLines: public Lines
{
public:
  StreamMapLines(std::istream& stream) : stream_(stream) {}
  [[nodiscard]] Lines::Iterator begin() override
  {

    constexpr size_t size = 100ULL*1024ULL*1048676ULL; // 100 GiB.
    auto buf = static_cast<char*>(mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_NORESERVE|MAP_ANONYMOUS|MAP_PRIVATE, -1, 0));
    if (buf == MAP_FAILED) {
      throw std::runtime_error(std::string("big mmap: ") + strerror(errno));
    }
    class ReadState {
    private:
      mutable std::mutex mu_;
      mutable std::condition_variable cv_;
      bool done_ = false;
      char* start_;
      char* end_;
    public:
      ReadState(char* buf): start_(buf), end_(buf){}
      void wait() const
      {
	std::unique_lock<std::mutex> lk(mu_);
	const auto old_end = end_;
	cv_.wait(lk, [this, old_end] {
		       return done_ || end_ != old_end;
		     });
      }
      void trigger() const
      {
	cv_.notify_one();
      }
      char* get_start() const
      {
	std::lock_guard<std::mutex> lk(mu_);
	return start_;
      }
      
      std::tuple<char*,char*,bool> get_start_end_done() const
      {
	std::lock_guard<std::mutex> lk(mu_);
	return {start_,end_,done_};
      }
      void set_start(char* v)
      {
	std::lock_guard<std::mutex> lk(mu_);
	start_ = v;
      }
      char* get_end() const
      {
	std::lock_guard<std::mutex> lk(mu_);
	return end_;
      }
      void set_end(char* v)
      {
	std::lock_guard<std::mutex> lk(mu_);
	end_ = v;
      }
      bool get_done() const
      {
	std::lock_guard<std::mutex> lk(mu_);
	return done_;
      }
      void set_done(bool v)
      {
	std::lock_guard<std::mutex> lk(mu_);
	done_ = v;
      }
    };
    auto state = std::make_shared<ReadState>(buf);
    std::thread th([buf, this, state]{
		     // Experimentally 8KiB seems to give best
		     // performance for the other method. Perhaps also
		     // in this method.
		     constexpr size_t chunk_size = 8192; 
		     for (;;) {
		       const auto [start,dst,done] = state->get_start_end_done();

		       // TODO: limit buffer utilization.
		       // TODO: check for end of buffer.
		       
		       stream_.read(dst, chunk_size);
		       const auto count = stream_.gcount();
		       state->set_end(dst + count);
		       state->trigger();
		       if (stream_.eof()) {
			 state->set_done(true);
			 break;
		       }
		       if (!stream_.good()) {
			 std::cerr << "Stopping read due to " << strerror(errno) << "\n";
			 state->set_done(true);
			 break;
		       }
		     }
		   });
    th.detach();
    return Lines::Iterator( [this, buf, state, last_unmap = buf, page_size = sysconf(_SC_PAGESIZE)]()mutable ->auto   {
			      constexpr size_t min_unmap_len = 1048576; // 1MiB.
			      for (;;) {
				const auto [start,end,done] = state->get_start_end_done();

				const size_t unmap_len = (start - last_unmap) & ~(page_size-1);
				if (unmap_len > min_unmap_len) {
				  if (munmap(last_unmap, unmap_len)) {
				    throw std::runtime_error("unmap failed");
				  }
				  last_unmap += unmap_len;
				}

				auto e = std::find(start, end, '\n');
				if (e == end) {
				  if (done) {
				    return std::make_pair(std::string_view{}, true);
				  }
				  state->wait();
				  continue;
				}
				const auto new_start = e+1;
				state->set_start(new_start);
				return std::make_pair(std::string_view{start, e}, false);
			      }
			    });

  }
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

