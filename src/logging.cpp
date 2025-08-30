#include "logging.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace hf {
namespace log {

namespace {
std::mutex log_mutex;
std::atomic<Level> current_level{Level::Info};

const char *level_to_string(Level lvl) {
  switch (lvl) {
  case Level::Debug:
    return "DEBUG";
  case Level::Info:
    return "INFO";
  case Level::Warn:
    return "WARN";
  case Level::Error:
    return "ERROR";
  }
  return "";
}

} // namespace

void init(Level level) { current_level.store(level); }

Level level_from_string(const std::string &name) {
  std::string s = name;
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  if (s == "debug")
    return Level::Debug;
  if (s == "warn")
    return Level::Warn;
  if (s == "error")
    return Level::Error;
  return Level::Info;
}

void log(Level level, const std::string &msg) {
  if (level < current_level.load())
    return;
  std::lock_guard<std::mutex> lock(log_mutex);
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm;
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  char buf[20];
  std::strftime(buf, sizeof(buf), "%F %T", &tm);
  std::cerr << "[" << buf << "][" << level_to_string(level)
            << "] " << msg << std::endl;
}

void debug(const std::string &msg) { log(Level::Debug, msg); }
void info(const std::string &msg) { log(Level::Info, msg); }
void warn(const std::string &msg) { log(Level::Warn, msg); }
void error(const std::string &msg) { log(Level::Error, msg); }

} // namespace log
} // namespace hf
