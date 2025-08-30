#include "rf_input.hpp"

#include <chrono>
#include <iostream>
#include <rtl-sdr.h>

namespace hf {

RfInput::RfInput() : dev_(nullptr) {
  presets_ = {{"80m JS8", 3578000u},
              {"40m FT8", 7074000u},
              {"40m JS8", 7078000u},
              {"20m FT8", 14074000u},
              {"20m JS8", 14078000u}};
  ring_buffer_.resize(kBasebandRate * 15); // 15 s of 12 kHz complex samples
}

RfInput::~RfInput() {
  stop();
  close();
}

bool RfInput::open(int device_index) {
  if (rtlsdr_open(&dev_, device_index) != 0) {
    std::cerr << "Failed to open RTL-SDR device\n";
    dev_ = nullptr;
    return false;
  }
  // Default configuration (240 kHz input, decimated to 12 kHz)
  set_sample_rate(kBasebandRate * kDecimation);
  set_band(0);
  return true;
}

void RfInput::close() {
  if (dev_) {
    rtlsdr_close(dev_);
    dev_ = nullptr;
  }
}

bool RfInput::set_frequency(uint32_t freq_hz) {
  if (!dev_)
    return false;
  return rtlsdr_set_center_freq(dev_, freq_hz) == 0;
}

bool RfInput::set_sample_rate(uint32_t rate_hz) {
  if (!dev_)
    return false;
  return rtlsdr_set_sample_rate(dev_, rate_hz) == 0;
}

bool RfInput::set_band(size_t index) {
  if (index >= presets_.size())
    return false;
  current_preset_ = index;
  return set_frequency(presets_[index].center_freq_hz);
}

bool RfInput::start() {
  if (!dev_ || running_)
    return false;
  running_ = true;
  worker_ = std::thread([this]() {
    rtlsdr_reset_buffer(dev_);
    rtlsdr_read_async(dev_, &RfInput::rtlsdr_callback, this, 0, 0);
  });
  return true;
}

void RfInput::stop() {
  if (running_) {
    running_ = false;
    rtlsdr_cancel_async(dev_);
    if (worker_.joinable()) {
      worker_.join();
    }
  }
}

void RfInput::rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {
  auto self = static_cast<RfInput *>(ctx);
  self->handle_samples(buf, len);
}

void RfInput::handle_samples(unsigned char *buf, uint32_t len) {
  // Down-convert/decimate to ~12 kHz complex baseband and store in ring buffer.
  std::vector<std::complex<float>> down;
  down.reserve(len / 2 / kDecimation + 1);
  std::complex<float> acc{0.0f, 0.0f};
  uint32_t count = 0;
  for (uint32_t i = 0; i + 1 < len; i += 2) {
    float i_val = (static_cast<int>(buf[i]) - 127.5f) / 127.5f;
    float q_val = (static_cast<int>(buf[i + 1]) - 127.5f) / 127.5f;
    acc += std::complex<float>(i_val, q_val);
    if (++count == kDecimation) {
      down.push_back(acc / static_cast<float>(kDecimation));
      acc = std::complex<float>{0.0f, 0.0f};
      count = 0;
    }
  }

  // Determine ring buffer position based on NTP-synchronised system clock.
  auto now = std::chrono::system_clock::now();
  auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
          .count();
  size_t pos = ((ms % 15000) * kBasebandRate) / 1000;
  pos = (pos + ring_buffer_.size() - down.size()) % ring_buffer_.size();

  std::lock_guard<std::mutex> lock(buffer_mutex_);
  for (auto &s : down) {
    ring_buffer_[pos] = s;
    pos = (pos + 1) % ring_buffer_.size();
  }
  ring_pos_ = pos;
}

std::vector<std::complex<float>> RfInput::snapshot() const {
  std::lock_guard<std::mutex> lock(buffer_mutex_);
  std::vector<std::complex<float>> out(ring_buffer_.size());
  size_t pos = ring_pos_;
  for (size_t i = 0; i < ring_buffer_.size(); ++i) {
    out[i] = ring_buffer_[(pos + i) % ring_buffer_.size()];
  }
  return out;
}

} // namespace hf
