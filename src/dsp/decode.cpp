#include "dsp/decode.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

extern "C" {
#include "ft8/ldpc.h"
#include "ft8/crc.h"
#include "ft8/constants.h"
}

namespace hf {

namespace {
const int kGrayDecode[8] = {0,1,3,2,6,4,5,7};

void pack_bits(const uint8_t bit_array[], int num_bits, uint8_t packed[]) {
  std::memset(packed, 0, (num_bits + 7) / 8);
  for (int i = 0; i < num_bits; ++i) {
    if (bit_array[i]) {
      packed[i/8] |= (uint8_t)1 << (7 - (i % 8));
    }
  }
}

uint32_t get_bits(const std::array<uint8_t,10>& data, int pos, int n) {
  uint32_t v = 0;
  for (int i = 0; i < n; ++i) {
    int bit_index = pos + i;
    int byte = bit_index / 8;
    int off = 7 - (bit_index % 8);
    v = (v << 1) | ((data[byte] >> off) & 1);
  }
  return v;
}

std::string unpack_call(uint32_t n) {
  const char base36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char base27[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char buf[7];
  int y3 = n % 27; n /= 27;
  int y2 = n % 27; n /= 27;
  int y1 = n % 27; n /= 27;
  int d  = n % 10; n /= 10;
  int x2 = n % 36; n /= 36;
  int x1 = n % 36;
  buf[0] = base36[x1];
  buf[1] = base36[x2];
  buf[2] = '0' + d;
  buf[3] = base27[y1];
  buf[4] = base27[y2];
  buf[5] = base27[y3];
  buf[6] = 0;
  std::string s(buf);
  while (!s.empty() && s.back() == ' ') s.pop_back();
  return s;
}

std::string unpack_grid(uint32_t n) {
  if (n >= 32400) return "";
  char g[5];
  int n0 = n;
  int d4 = n0 % 10; n0 /= 10;
  int d3 = n0 % 10; n0 /= 10;
  int l2 = n0 % 18; n0 /= 18;
  int l1 = n0 % 18;
  g[0] = 'A' + l1;
  g[1] = 'A' + l2;
  g[2] = '0' + d3;
  g[3] = '0' + d4;
  g[4] = 0;
  return std::string(g);
}

std::string decode_ft8_payload(const std::array<uint8_t,10>& payload) {
  uint32_t n1 = get_bits(payload, 0, 28);
  uint32_t n2 = get_bits(payload, 28, 28);
  uint32_t n3 = get_bits(payload, 56, 15);
  uint32_t type = get_bits(payload, 71, 6);
  if (type != 0) return ""; // only handle standard messages
  std::string c1 = unpack_call(n1);
  std::string c2 = unpack_call(n2);
  std::string msg;
  if (n3 < 32400) {
    msg = c1 + " " + c2 + " " + unpack_grid(n3);
  } else {
    msg = c1 + " " + c2;
  }
  return msg;
}

std::string decode_js8_payload(const std::array<uint8_t,10>& payload) {
  std::string out;
  for (int i = 0; i < 11; ++i) {
    uint32_t v = get_bits(payload, i * 7, 7);
    char c = static_cast<char>(v);
    if (c == 0) break;
    if (!std::isprint(static_cast<unsigned char>(c))) return "";
    out.push_back(c);
  }
  return out;
}

} // namespace

DecodedMessage LDPCDecoder::decode(const std::vector<int> &tones,
                                   bool allow_js8) const {
  float llr[FTX_LDPC_N] = {0};
  int bit_idx = 0;
  for (int i = 0; i < (int)tones.size(); ++i) {
    if ((i < 7) || (i >= 36 && i < 43) || (i >= 72 && i < 79))
      continue; // skip Costas sync tones
    int symbol = tones[i];
    if (symbol < 0 || symbol > 7) continue;
    int bits3 = kGrayDecode[symbol];
    for (int b = 2; b >= 0 && bit_idx < FTX_LDPC_N; --b) {
      int bit = (bits3 >> b) & 1;
      llr[bit_idx++] = bit ? -1.0f : 1.0f;
    }
  }

  uint8_t plain[FTX_LDPC_N];
  int errors = 0;
  bp_decode(llr, 50, plain, &errors);

  DecodedMessage msg{};
  msg.ldpc_errors = errors;
  msg.mode = Mode::FT8;
  if (errors > 0) {
    msg.crc_ok = false;
    return msg;
  }

  uint8_t a91[FTX_LDPC_K_BYTES];
  pack_bits(plain, FTX_LDPC_K, a91);
  uint16_t extracted = ftx_extract_crc(a91);
  a91[9] &= 0xF8u;
  a91[10] = 0;
  uint16_t calc = ftx_compute_crc(a91, 96 - 14);
  msg.crc_ok = (extracted == calc);
  std::copy(a91, a91 + 10, msg.payload.begin());
  if (msg.crc_ok) {
    msg.text = decode_ft8_payload(msg.payload);
    if (allow_js8 && msg.text.empty()) {
      msg.text = decode_js8_payload(msg.payload);
      if (!msg.text.empty())
        msg.mode = Mode::JS8;
    }
  }
  return msg;
}

} // namespace hf

