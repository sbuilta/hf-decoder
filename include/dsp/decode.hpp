#pragma once
#include "dsp/demod.hpp"
#include <array>
#include <string>
#include <vector>

namespace hf {

struct DecodedMessage {
  bool crc_ok;
  int ldpc_errors;
  std::array<uint8_t, 10> payload; // first 77 bits packed MSB-first
  std::string text;                // decoded message text (FT8/JS8)
};

class LDPCDecoder {
public:
  DecodedMessage decode(const std::vector<int> &tones,
                        bool allow_js8 = true) const;
};

} // namespace hf

