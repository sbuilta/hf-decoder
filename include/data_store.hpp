#pragma once
#include <sqlite3.h>
#include <cstdint>
#include <string>
#include <vector>
#include "dsp/engine.hpp"

namespace hf {

struct DbRecord {
  int64_t timestamp; // Unix epoch seconds
  std::string band;
  double frequency_hz;
  Mode mode;
  float snr_db;
  std::string text;
};

class DataStore {
public:
  explicit DataStore(const std::string &path);
  ~DataStore();

  bool open();
  void close();
  bool init();
  bool insert(const std::vector<DbRecord> &recs);
  std::vector<DbRecord> recent(int limit);

private:
  std::string path_;
  sqlite3 *db_;
};

std::string mode_to_string(Mode m);

} // namespace hf
