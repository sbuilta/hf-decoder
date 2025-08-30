#pragma once
#include "dsp/sync.hpp"
#include "dsp/demod.hpp"
#include "dsp/decode.hpp"
#include <complex>
#include <string>
#include <vector>

namespace hf {

struct DecodedSignal {
  float freq_hz;
  float time_sec;
  float snr_db;
  Mode mode;
  bool crc_ok;
  int ldpc_errors;
  std::string text;
};

class DecodeEngine {
public:
  explicit DecodeEngine(uint32_t sample_rate = 12000,
                        bool enable_js8 = true);
  std::vector<DecodedSignal>
  process(const std::vector<std::complex<float>> &frame) const;
  void set_js8_enabled(bool en) { js8_enabled_ = en; }
  bool js8_enabled() const { return js8_enabled_; }

private:
  bool js8_enabled_;
  SyncDetector sync_;
  FSK8Demod demod_;
  LDPCDecoder decoder_;
};

} // namespace hf

