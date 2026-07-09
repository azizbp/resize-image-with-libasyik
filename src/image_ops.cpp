#include "image_ops.hpp"

#include <stdexcept>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace image_ops
{
cv::Mat decode(const std::vector<std::uint8_t> &bytes)
{
  return cv::imdecode(bytes, cv::IMREAD_COLOR);
}

cv::Mat resize(const cv::Mat &image, int width, int height)
{
  cv::Mat resized;
  cv::resize(image, resized, cv::Size(width, height));
  return resized;
}

std::vector<std::uint8_t> encode_jpeg(const cv::Mat &image)
{
  std::vector<std::uint8_t> bytes;
  if (!cv::imencode(".jpg", image, bytes))
    throw std::runtime_error("failed to encode image as JPEG");
  return bytes;
}
}  // namespace image_ops
