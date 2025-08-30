#pragma once
#include <string>

namespace hf {
namespace log {

enum class Level { Debug = 0, Info, Warn, Error };

void init(Level level = Level::Info);
Level level_from_string(const std::string &name);
void log(Level level, const std::string &msg);
void debug(const std::string &msg);
void info(const std::string &msg);
void warn(const std::string &msg);
void error(const std::string &msg);

} // namespace log
} // namespace hf
