#include "rf_input.hpp"
#include <iostream>
#include <rtl-sdr.h>

namespace hf {

RfInput::RfInput() : dev_(nullptr) {
  presets_ = {{"80m JS8", 3578000u},
              {"40m FT8", 7074000u},
              {"40m JS8", 7078000u},
              {"20m FT8", 14074000u},
              {"20m JS8", 14078000u}};
  ring_buffer_.resize(12'000 * 15); // 15 s of 12 kHz complex samples
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
  // Default configuration
  set_sample_rate(240000);
  set_frequency(presets_.front().center_freq_hz);
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
  // Convert unsigned IQ samples to float complex and store in ring buffer.
  std::lock_guard<std::mutex> lock(buffer_mutex_);
  for (uint32_t i = 0; i + 1 < len; i += 2) {
    float i_val = (static_cast<int>(buf[i]) - 127.5f) / 127.5f;
    float q_val = (static_cast<int>(buf[i + 1]) - 127.5f) / 127.5f;
    ring_buffer_[ring_pos_] = {i_val, q_val};
    ring_pos_ = (ring_pos_ + 1) % ring_buffer_.size();
  }
}

} // namespace hf
