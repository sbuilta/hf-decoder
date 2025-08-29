#include "data_store.hpp"
#include <iostream>

namespace hf {

std::string mode_to_string(Mode m) {
  return m == Mode::JS8 ? "JS8" : "FT8";
}

DataStore::DataStore(const std::string &path) : path_(path), db_(nullptr) {}
DataStore::~DataStore() { close(); }

bool DataStore::open() {
  if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
    db_ = nullptr;
    return false;
  }
  return true;
}

void DataStore::close() {
  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

bool DataStore::init() {
  const char *sql =
      "CREATE TABLE IF NOT EXISTS messages ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "timestamp INTEGER,"
      "band TEXT,"
      "frequency REAL,"
      "mode TEXT,"
      "snr REAL,"
      "text TEXT);";
  char *err = nullptr;
  int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    sqlite3_free(err);
    return false;
  }
  return true;
}

bool DataStore::insert(const std::vector<DbRecord> &recs) {
  const char *sql =
      "INSERT INTO messages (timestamp,band,frequency,mode,snr,text)"
      " VALUES (?,?,?,?,?,?);";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

  if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    sqlite3_finalize(stmt);
    return false;
  }
  bool ok = true;
  for (const auto &r : recs) {
    sqlite3_bind_int64(stmt, 1, r.timestamp);
    sqlite3_bind_text(stmt, 2, r.band.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, r.frequency_hz);
    std::string mode = mode_to_string(r.mode);
    sqlite3_bind_text(stmt, 4, mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, r.snr_db);
    sqlite3_bind_text(stmt, 6, r.text.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
      ok = false;
      break;
    }
    sqlite3_reset(stmt);
  }
  if (ok)
    sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
  else
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
  sqlite3_finalize(stmt);
  return ok;
}

std::vector<DbRecord> DataStore::recent(int limit) {
  std::vector<DbRecord> out;
  std::string sql =
      "SELECT timestamp,band,frequency,mode,snr,text FROM messages ORDER BY id DESC LIMIT ?;";
  sqlite3_stmt *stmt = nullptr;
  if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    return out;
  sqlite3_bind_int(stmt, 1, limit);
  int rc;
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    DbRecord r{};
    r.timestamp = sqlite3_column_int64(stmt, 0);
    const unsigned char *band = sqlite3_column_text(stmt, 1);
    if (band) r.band = reinterpret_cast<const char *>(band);
    r.frequency_hz = sqlite3_column_double(stmt, 2);
    const unsigned char *mode = sqlite3_column_text(stmt, 3);
    std::string mode_str = mode ? reinterpret_cast<const char *>(mode) : "FT8";
    r.mode = mode_str == "JS8" ? Mode::JS8 : Mode::FT8;
    r.snr_db = static_cast<float>(sqlite3_column_double(stmt, 4));
    const unsigned char *text = sqlite3_column_text(stmt, 5);
    if (text) r.text = reinterpret_cast<const char *>(text);
    out.push_back(std::move(r));
  }
  if (rc != SQLITE_DONE)
    out.clear();
  sqlite3_finalize(stmt);
  return out;
}

} // namespace hf
