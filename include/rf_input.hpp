#pragma once
#include <atomic>
#include <complex>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct rtlsdr_dev; // forward declaration from librtlsdr

namespace hf {

struct BandPreset {
  const char *name;
  uint32_t center_freq_hz;
};

class RfInput {
public:
  RfInput();
  ~RfInput();

  bool open(int device_index = 0);
  void close();

  bool set_frequency(uint32_t freq_hz);
  bool set_sample_rate(uint32_t rate_hz);

  // Set active band preset by index. Returns false on invalid index or
  // failure to retune the SDR.
  bool set_band(size_t index);
  size_t current_band() const { return current_preset_; }

  bool start();
  void stop();

  const std::vector<BandPreset> &presets() const { return presets_; }
  const std::vector<std::complex<float>> &ring_buffer() const {
    return ring_buffer_;
  }
  size_t ring_pos() const { return ring_pos_; }

  // Return a copy of the current 15 s ring buffer starting at the
  // most recent sample. Thread-safe snapshot for external consumers.
  std::vector<std::complex<float>> snapshot() const;


private:
  static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx);
  void handle_samples(unsigned char *buf, uint32_t len);

  rtlsdr_dev *dev_;
  std::thread worker_;
  std::atomic<bool> running_{false};

  std::vector<std::complex<float>> ring_buffer_;
  size_t ring_pos_{};
  mutable std::mutex buffer_mutex_;

  static constexpr uint32_t kBasebandRate = 12000; // 12 kHz
  static constexpr uint32_t kDecimation = 20;      // 240 kHz / 20 = 12 kHz

  std::vector<BandPreset> presets_;
  size_t current_preset_{};
};

} // namespace hf
