#include "rf_input.hpp"
#include <iostream>

int main() {
  hf::RfInput rf;
  std::cout << "HF FT8/JS8 Decoder skeleton\n";
  std::cout << "Available band presets:" << std::endl;
  for (const auto &p : rf.presets()) {
    std::cout << " - " << p.name << " (" << p.center_freq_hz << " Hz)"
              << std::endl;
  }
  if (!rf.open()) {
    std::cout << "RTL-SDR device not found" << std::endl;
  }
  return 0;
}
