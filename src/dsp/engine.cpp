#include "dsp/engine.hpp"
#include <future>

namespace hf {

DecodeEngine::DecodeEngine(uint32_t sample_rate, bool enable_js8)
    : js8_enabled_(enable_js8), sync_(sample_rate), demod_(sample_rate) {}

std::vector<DecodedSignal>
DecodeEngine::process(const std::vector<std::complex<float>> &frame) const {
  std::vector<DecodedSignal> results;
  auto cands = sync_.detect(frame);
  std::vector<std::future<DecodedSignal>> futures;
  futures.reserve(cands.size());
  for (const auto &cand : cands) {
    futures.emplace_back(std::async(std::launch::async, [this, &frame, cand]() {
      DecodedSignal res{};
      auto sig = demod_.demodulate(frame, cand);
      res.freq_hz = sig.freq_hz;
      res.time_sec = sig.time_sec;
      res.snr_db = sig.snr_db;
      auto msg = decoder_.decode(sig.tones, js8_enabled_);
      res.mode = msg.mode;
      res.crc_ok = msg.crc_ok;
      res.ldpc_errors = msg.ldpc_errors;
      res.text = msg.text;
      return res;
    }));
  }
  results.reserve(futures.size());
  for (auto &f : futures) {
    results.push_back(f.get());
  }
  return results;
}

} // namespace hf

