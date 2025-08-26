#include "dsp/sync.hpp"

#include <algorithm>
#include <fftw3.h>
#include <numeric>

namespace hf {

namespace {
// 7x7 Costas array used by FT8/JS8
constexpr int kCostasSeq[7] = {0, 1, 3, 2, 4, 6, 5};
} // namespace

SyncDetector::SyncDetector(uint32_t sample_rate)
    : sample_rate_(sample_rate) {
  // FT8/JS8 symbol is 160 ms -> 12000 * 0.160 = 1920 samples
  symbol_len_ = static_cast<int>(sample_rate_ / 6.25f); // 1920 at 12 kHz
}

std::vector<SyncCandidate>
SyncDetector::detect(const std::vector<std::complex<float>> &frame) const {
  std::vector<SyncCandidate> candidates;
  if (frame.size() < static_cast<size_t>(symbol_len_ * 7))
    return candidates;

  const int fft_size = symbol_len_;
  std::vector<std::complex<float>> tmp(fft_size);
  std::vector<std::complex<float>> fft_out(fft_size);
  fftwf_plan plan = fftwf_plan_dft_1d(
      fft_size, reinterpret_cast<fftwf_complex *>(tmp.data()),
      reinterpret_cast<fftwf_complex *>(fft_out.data()), FFTW_FORWARD,
      FFTW_ESTIMATE);

  const int step = symbol_len_ / 2; // 80 ms step
  const int max_t = static_cast<int>(frame.size()) - fft_size * 7;

  for (int t = 0; t <= max_t; t += step) {
    // FFT magnitudes for 7 symbols
    std::vector<std::vector<float>> mags(7, std::vector<float>(fft_size));
    for (int i = 0; i < 7; ++i) {
      std::copy(frame.begin() + t + i * fft_size,
                frame.begin() + t + (i + 1) * fft_size, tmp.begin());
      fftwf_execute(plan);
      for (int k = 0; k < fft_size; ++k) {
        float re = fft_out[k].real();
        float im = fft_out[k].imag();
        mags[i][k] = re * re + im * im;
      }
    }

    // Evaluate correlation for each frequency bin (0..fs/2)
    const int max_bin = fft_size / 2 - 8; // leave room for Costas offsets
    std::vector<float> metric(max_bin);
    for (int k = 0; k < max_bin; ++k) {
      float sum = 0.0f;
      for (int i = 0; i < 7; ++i)
        sum += mags[i][k + kCostasSeq[i]];
      metric[k] = sum;
    }
    float avg = std::accumulate(metric.begin(), metric.end(), 0.0f) /
                static_cast<float>(metric.size());
    for (int k = 0; k < max_bin; ++k) {
      if (metric[k] > avg * 10.0f) { // heuristic threshold
        SyncCandidate c;
        c.freq_hz = (static_cast<float>(k) * sample_rate_) / fft_size;
        c.time_sec = static_cast<float>(t) / sample_rate_;
        c.metric = metric[k];
        candidates.push_back(c);
      }
    }
  }

  fftwf_destroy_plan(plan);
  std::sort(candidates.begin(), candidates.end(),
            [](const SyncCandidate &a, const SyncCandidate &b) {
              return a.metric > b.metric;
            });
  return candidates;
}

} // namespace hf

