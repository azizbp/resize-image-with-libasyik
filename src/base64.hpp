#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// Small self-contained base64 codec (standard alphabet, '=' padding).
// Kept dependency-free on purpose
namespace base64
{
inline constexpr const char *alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline std::string encode(const std::vector<std::uint8_t> &data)
{
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);

  std::size_t i = 0;
  for (; i + 3 <= data.size(); i += 3) {
    std::uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
    out.push_back(alphabet[(n >> 18) & 0x3F]);
    out.push_back(alphabet[(n >> 12) & 0x3F]);
    out.push_back(alphabet[(n >> 6) & 0x3F]);
    out.push_back(alphabet[n & 0x3F]);
  }

  // trailing 1 or 2 bytes -> pad with '='
  std::size_t rem = data.size() - i;
  if (rem == 1) {
    std::uint32_t n = data[i] << 16;
    out.push_back(alphabet[(n >> 18) & 0x3F]);
    out.push_back(alphabet[(n >> 12) & 0x3F]);
    out += "==";
  } else if (rem == 2) {
    std::uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
    out.push_back(alphabet[(n >> 18) & 0x3F]);
    out.push_back(alphabet[(n >> 12) & 0x3F]);
    out.push_back(alphabet[(n >> 6) & 0x3F]);
    out.push_back('=');
  }
  return out;
}

inline int decode_value(char c)
{
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;  // invalid character
}

// Strict decoder: throws std::invalid_argument on any malformed input.
inline std::vector<std::uint8_t> decode(const std::string &text)
{
  if (text.empty() || text.size() % 4 != 0)
    throw std::invalid_argument(
        "base64: length must be a positive multiple of 4");

  // '=' is only valid as the last one or two characters
  std::size_t pad = 0;
  if (text[text.size() - 1] == '=') pad++;
  if (text[text.size() - 2] == '=') pad++;

  std::vector<std::uint8_t> out;
  out.reserve(text.size() / 4 * 3);

  for (std::size_t i = 0; i < text.size(); i += 4) {
    std::uint32_t n = 0;
    std::size_t group_pad = 0;
    for (std::size_t j = 0; j < 4; j++) {
      char c = text[i + j];
      if (c == '=') {
        if (i + j < text.size() - pad)
          throw std::invalid_argument("base64: unexpected '=' padding");
        group_pad++;
        n <<= 6;
      } else {
        int v = decode_value(c);
        if (v < 0) throw std::invalid_argument("base64: invalid character");
        n = (n << 6) | static_cast<std::uint32_t>(v);
      }
    }
    out.push_back((n >> 16) & 0xFF);
    if (group_pad < 2) out.push_back((n >> 8) & 0xFF);
    if (group_pad < 1) out.push_back(n & 0xFF);
  }
  return out;
}
}  // namespace base64
