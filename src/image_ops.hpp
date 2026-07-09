#pragma once

#include <cstdint>
#include <vector>

#include <opencv2/core.hpp>

// Thin OpenCV wrappers, kept free of any HTTP/JSON dependency so they are
// unit-testable in isolation
namespace image_ops
{
// Decode image bytes (JPEG). Returns an empty Mat when the bytes are not a
// decodable image.
cv::Mat decode(const std::vector<std::uint8_t> &bytes);

// Resize to exactly width x height.
cv::Mat resize(const cv::Mat &image, int width, int height);

// Encode back to JPEG bytes. Throws std::runtime_error on failure.
std::vector<std::uint8_t> encode_jpeg(const cv::Mat &image);
}  // namespace image_ops
