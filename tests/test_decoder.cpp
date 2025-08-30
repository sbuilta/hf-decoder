#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "dsp/decode.hpp"
extern "C" {
#include "ft8/crc.h"
}
#include <array>
#include <fstream>
#include <vector>

std::array<uint8_t,10> read_payload(const std::string &path) {
  std::array<uint8_t,10> data{};
  std::ifstream f(path, std::ios::binary);
  f.read(reinterpret_cast<char*>(data.data()), data.size());
  return data;
}

TEST_CASE("FT8 payload decodes correctly") {
  auto payload = read_payload("tests/samples/ft8_payload.bin");
  std::array<uint8_t,12> a91{};
  ftx_add_crc(payload.data(), a91.data());
  auto crc = ftx_extract_crc(a91.data());
  a91[9] &= 0xF8u;
  a91[10] = 0;
  REQUIRE(crc == ftx_compute_crc(a91.data(), 96 - 14));
  REQUIRE(hf::decode_ft8_payload(payload) == "KA1ABC WA9XYZ EM00");
}

TEST_CASE("JS8 payload decodes correctly") {
  auto payload = read_payload("tests/samples/js8_payload.bin");
  REQUIRE(hf::decode_js8_payload(payload) == "HELLO");
}

