#include "dsp/demod.hpp"

#include <algorithm>
#include <cmath>
#include <fftw3.h>

namespace hf {

namespace {
constexpr int kCostasSeq[7] = {0, 1, 3, 2, 4, 6, 5};
}

FSK8Demod::FSK8Demod(uint32_t sample_rate)
    : sample_rate_(sample_rate) {
  symbol_len_ = static_cast<int>(sample_rate_ / 6.25f);
}

DemodulatedSignal FSK8Demod::demodulate(
    const std::vector<std::complex<float>> &frame,
    const SyncCandidate &cand) const {
  DemodulatedSignal out{};
  out.freq_hz = cand.freq_hz;
  out.time_sec = cand.time_sec;

  if (frame.empty())
    return out;

  int base_bin = static_cast<int>(cand.freq_hz * symbol_len_ / sample_rate_);
  int t0 = static_cast<int>(cand.time_sec * sample_rate_);
  if (t0 < 0 || t0 + symbol_len_ * 79 > static_cast<int>(frame.size()))
    return out;

  std::vector<std::complex<float>> tmp(symbol_len_);
  std::vector<std::complex<float>> fft_out(symbol_len_);
  fftwf_plan plan = fftwf_plan_dft_1d(
      symbol_len_, reinterpret_cast<fftwf_complex *>(tmp.data()),
      reinterpret_cast<fftwf_complex *>(fft_out.data()), FFTW_FORWARD,
      FFTW_ESTIMATE);

  // Frequency refinement around initial estimate
  std::vector<std::vector<float>> mags(7, std::vector<float>(symbol_len_));
  for (int i = 0; i < 7; ++i) {
    std::copy(frame.begin() + t0 + i * symbol_len_,
              frame.begin() + t0 + (i + 1) * symbol_len_, tmp.begin());
    fftwf_execute(plan);
    for (int k = 0; k < symbol_len_; ++k) {
      float re = fft_out[k].real();
      float im = fft_out[k].imag();
      mags[i][k] = re * re + im * im;
    }
  }
  float best_metric = -1.0f;
  int best_bin = base_bin;
  for (int b = base_bin - 2; b <= base_bin + 2; ++b) {
    if (b < 0 || b + 7 >= symbol_len_)
      continue;
    float sum = 0.0f;
    for (int i = 0; i < 7; ++i)
      sum += mags[i][b + kCostasSeq[i]];
    if (sum > best_metric) {
      best_metric = sum;
      best_bin = b;
    }
  }

  // Time refinement around start
  int best_dt = 0;
  for (int dt = -symbol_len_ / 2; dt <= symbol_len_ / 2; dt += symbol_len_ / 8) {
    int start = t0 + dt;
    if (start < 0 || start + symbol_len_ * 7 > static_cast<int>(frame.size()))
      continue;
    float sum = 0.0f;
    for (int i = 0; i < 7; ++i) {
      std::copy(frame.begin() + start + i * symbol_len_,
                frame.begin() + start + (i + 1) * symbol_len_, tmp.begin());
      fftwf_execute(plan);
      int bin = best_bin + kCostasSeq[i];
      float re = fft_out[bin].real();
      float im = fft_out[bin].imag();
      sum += re * re + im * im;
    }
    if (sum > best_metric) {
      best_metric = sum;
      best_dt = dt;
    }
  }

  int start = t0 + best_dt;
  out.freq_hz = static_cast<float>(best_bin) * sample_rate_ / symbol_len_;
  out.time_sec = static_cast<float>(start) / sample_rate_;

  // Demodulate 79 symbols and measure SNR
  out.tones.resize(79);
  out.snr_db = 0.0f;
  float sig_pow = 0.0f;
  float noise_pow = 0.0f;
  int sym_cnt = 0;
  for (int s = 0; s < 79; ++s) {
    int off = start + s * symbol_len_;
    if (off + symbol_len_ > static_cast<int>(frame.size())) {
      out.tones.resize(s);
      break;
    }
    std::copy(frame.begin() + off, frame.begin() + off + symbol_len_,
              tmp.begin());
    fftwf_execute(plan);
    int best_tone = 0;
    float max_p = 0.0f;
    float tone_p[8];
    for (int tone = 0; tone < 8; ++tone) {
      int bin = best_bin + tone;
      float re = fft_out[bin].real();
      float im = fft_out[bin].imag();
      float p = re * re + im * im;
      tone_p[tone] = p;
      if (p > max_p) {
        max_p = p;
        best_tone = tone;
      }
    }
    sig_pow += tone_p[best_tone];
    for (int tone = 0; tone < 8; ++tone) {
      if (tone != best_tone)
        noise_pow += tone_p[tone];
    }
    ++sym_cnt;
    out.tones[s] = best_tone;
  }

  if (sym_cnt > 0) {
    float avg_sig = sig_pow / sym_cnt;
    float avg_noise = noise_pow / (sym_cnt * 7);
    float bin_bw = static_cast<float>(sample_rate_) / symbol_len_;
    float noise_ref = avg_noise * (2500.0f / bin_bw);
    if (noise_ref > 0.0f)
      out.snr_db = 10.0f * std::log10(avg_sig / noise_ref);
  }

  fftwf_destroy_plan(plan);
  return out;
}

} // namespace hf

