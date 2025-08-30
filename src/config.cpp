#include "config.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace hf {
namespace {
std::string trim(const std::string &s) {
  auto start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
    return "";
  auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}
} // namespace

Config Config::load(const std::string &path) {
  Config cfg;
  std::ifstream in(path);
  if (!in.is_open())
    return cfg;
  std::string line;
  while (std::getline(in, line)) {
    auto hash = line.find('#');
    if (hash != std::string::npos)
      line = line.substr(0, hash);
    line = trim(line);
    if (line.empty())
      continue;
    auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = trim(line.substr(0, eq));
    std::string value = trim(line.substr(eq + 1));
    if (key == "db_path") {
      cfg.db_path = value;
    } else if (key == "web_port") {
      cfg.web_port = std::stoi(value);
    } else if (key == "log_level") {
      cfg.log_level = value;
    }
  }
  return cfg;
}

} // namespace hf
