#include "rf_input.hpp"
#include "dsp/engine.hpp"
#include "data_store.hpp"
#include <ctime>
#include <iostream>

int main() {
  hf::RfInput rf;
  hf::DecodeEngine engine(/*sample_rate=*/12000, /*enable_js8=*/true);

  std::cout << "HF FT8/JS8 Decoder skeleton\n";
  std::cout << "Available band presets:" << std::endl;
  for (const auto &p : rf.presets()) {
    std::cout << " - " << p.name << " (" << p.center_freq_hz << " Hz)"
              << std::endl;
  }

  // Attempt to open RTL-SDR; okay if not present during development.
  if (!rf.open()) {
    std::cout << "RTL-SDR device not found" << std::endl;
  }

  // Run a dry-run decode over the current ring buffer.
  auto results = engine.process(rf.ring_buffer());
  std::cout << "Decoded signals: " << results.size() << std::endl;
  for (const auto &r : results) {
    std::cout << "freq=" << r.freq_hz << " Hz time=" << r.time_sec
              << " s SNR=" << r.snr_db << " dB CRC="
              << (r.crc_ok ? "OK" : "FAIL")
              << " LDPC errors=" << r.ldpc_errors;
    if (!r.text.empty()) {
      std::cout << " msg=\"" << r.text << "\"";
    }
    std::cout << std::endl;
  }

  hf::DataStore db("decodes.db");
  if (db.open() && db.init()) {
    std::vector<hf::DbRecord> recs;
    recs.reserve(results.size());
    for (const auto &r : results) {
      hf::DbRecord rec{};
      rec.timestamp = std::time(nullptr);
      rec.band = "unknown";
      rec.frequency_hz = r.freq_hz;
      rec.mode = r.mode;
      rec.snr_db = r.snr_db;
      rec.text = r.text;
      recs.push_back(std::move(rec));
    }
    if (!recs.empty()) {
      db.insert(recs);
      auto recent = db.recent(5);
      std::cout << "Recent messages: " << recent.size() << std::endl;
    }
    db.close();
  }

  return 0;
}
