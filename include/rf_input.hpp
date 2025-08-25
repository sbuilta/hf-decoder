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

  bool start();
  void stop();

  const std::vector<BandPreset> &presets() const { return presets_; }

private:
  static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx);
  void handle_samples(unsigned char *buf, uint32_t len);

  rtlsdr_dev *dev_;
  std::thread worker_;
  std::atomic<bool> running_{false};

  std::vector<std::complex<float>> ring_buffer_;
  size_t ring_pos_{};
  std::mutex buffer_mutex_;

  std::vector<BandPreset> presets_;
};

} // namespace hf
