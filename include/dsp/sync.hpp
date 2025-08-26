#pragma once
#include <complex>
#include <cstdint>
#include <vector>

namespace hf {

struct SyncCandidate {
  float freq_hz;    // frequency relative to baseband center
  float time_sec;   // time offset from start of frame
  float metric;     // correlation strength
};

class SyncDetector {
public:
  explicit SyncDetector(uint32_t sample_rate = 12000);
  std::vector<SyncCandidate>
  detect(const std::vector<std::complex<float>> &frame) const;

private:
  uint32_t sample_rate_;
  int symbol_len_;
};

} // namespace hf

