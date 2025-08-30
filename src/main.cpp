#include "rf_input.hpp"
#include "dsp/engine.hpp"
#include "data_store.hpp"
#include "web_server.hpp"
#include "thread_safe_queue.hpp"
#include "config.hpp"
#include "logging.hpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  auto cfg = hf::Config::load("hfdecoder.conf");
  hf::log::init(hf::log::level_from_string(cfg.log_level));

  hf::RfInput rf;
  hf::DecodeEngine engine(/*sample_rate=*/12000, /*enable_js8=*/true);
  hf::DataStore db(cfg.db_path);
  if (!db.open() || !db.init()) {
    hf::log::error("Failed to open database");
    return 1;
  }

  hf::log::info("HF FT8/JS8 Decoder starting");
  hf::log::info("Available band presets:");
  for (const auto &p : rf.presets()) {
    hf::log::info(std::string(" - ") + p.name + " (" +
                  std::to_string(p.center_freq_hz) + " Hz)");
  }

  std::atomic<bool> running{true};
  hf::ThreadSafeQueue<std::vector<std::complex<float>>> decode_queue;
  hf::ThreadSafeQueue<std::vector<hf::DbRecord>> log_queue;

  // Logger thread writes decoded records to the database.
  std::thread logger([&]() {
    std::vector<hf::DbRecord> batch;
    while (log_queue.pop(batch)) {
      db.insert(batch);
    }
  });

  // Web server runs in its own thread using CivetWeb's internal loop.
  std::thread server_thread([&]() {
    hf::WebServer server(db, rf, engine, "docs/web", cfg.web_port);
    while (running) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  });

  // Attempt to open RTL-SDR; okay if not present during development.
  if (!rf.open()) {
    hf::log::warn("RTL-SDR device not found");
  } else {
    rf.start();
  }

  // Capture thread snapshots the latest 15 s frame for decoding.
  std::thread capture([&]() {
    while (running) {
      auto frame = rf.snapshot();
      decode_queue.push(std::move(frame));
      std::this_thread::sleep_for(std::chrono::seconds(15));
    }
  });

  // Decoder thread processes frames from the capture queue.
  std::thread decoder([&]() {
    std::vector<std::complex<float>> frame;
    while (decode_queue.pop(frame)) {
      auto results = engine.process(frame);
      std::vector<hf::DbRecord> recs;
      recs.reserve(results.size());
      auto now = std::time(nullptr);
      for (const auto &r : results) {
        hf::DbRecord rec{};
        rec.timestamp = now;
        rec.band = "unknown";
        rec.frequency_hz = r.freq_hz;
        rec.mode = r.mode;
        rec.snr_db = r.snr_db;
        rec.text = r.text;
        recs.push_back(rec);
      }
      if (!recs.empty()) {
        log_queue.push(std::move(recs));
      }
    }
  });

  // Run for one decode cycle in this demo.
  std::this_thread::sleep_for(std::chrono::seconds(15));
  running = false;

  capture.join();
  decode_queue.stop();
  decoder.join();
  log_queue.stop();
  logger.join();
  server_thread.join();

  rf.stop();
  db.close();
  return 0;
}
