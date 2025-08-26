#pragma once
#include "dsp/sync.hpp"
#include <complex>
#include <cstdint>
#include <vector>

namespace hf {

struct DemodulatedSignal {
  float freq_hz;  // refined frequency
  float time_sec; // refined time offset
  float snr_db;   // SNR referenced to 2.5 kHz noise BW
  std::vector<int> tones; // 79 symbols
};

class FSK8Demod {
public:
  explicit FSK8Demod(uint32_t sample_rate = 12000);
  DemodulatedSignal demodulate(const std::vector<std::complex<float>> &frame,
                               const SyncCandidate &cand) const;

private:
  uint32_t sample_rate_;
  int symbol_len_;
};

} // namespace hf

