#pragma once
#include <string>

namespace hf {

struct Config {
  std::string db_path = "decodes.db";
  int web_port = 8080;
  std::string log_level = "info";

  static Config load(const std::string &path);
};

} // namespace hf
