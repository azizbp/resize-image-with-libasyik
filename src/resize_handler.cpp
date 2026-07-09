#include "resize_handler.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>

#include "base64.hpp"
#include "image_ops.hpp"

namespace resize_service
{
namespace
{
using nlohmann::json;

// Sane upper bound on output dimensions to avoid excessive memory use
constexpr int kMaxDimension = 8192;

// Exception carrying the HTTP status of a client (4xx) error; anything
// else escaping the flow below becomes a 500.
struct client_error : std::runtime_error {
  int status;
  client_error(int status_, const std::string &what_)
      : std::runtime_error(what_), status(status_)
  {
  }
};

handler_result error_response(int status, const std::string &message)
{
  // Failure contract uses capital-M "Message"
  json body = {{"code", status}, {"Message", message}};
  return {status, body.dump()};
}

int get_positive_dimension(const json &request, const char *field)
{
  if (!request.contains(field))
    throw client_error(400, std::string("missing field: ") + field);
  const auto &value = request.at(field);
  if (!value.is_number_integer())
    throw client_error(400, std::string(field) + " must be an integer");
  int dimension = value.get<int>();
  if (dimension <= 0)
    throw client_error(400, std::string(field) + " must be positive");
  if (dimension > kMaxDimension)
    throw client_error(400, std::string(field) + " must be <= " +
                                std::to_string(kMaxDimension));
  return dimension;
}
}  // namespace

handler_result handle_resize_request(const std::string &request_body)
{
  try {
    json request = json::parse(request_body, nullptr, /*allow_exceptions=*/false);
    if (request.is_discarded() || !request.is_object())
      throw client_error(400, "request body is not a valid JSON object");

    if (!request.contains("input_jpeg") || !request.at("input_jpeg").is_string())
      throw client_error(400, "missing or non-string field: input_jpeg");
    int width = get_positive_dimension(request, "desired_width");
    int height = get_positive_dimension(request, "desired_height");

    std::vector<std::uint8_t> jpeg_bytes;
    try {
      jpeg_bytes = base64::decode(request.at("input_jpeg").get<std::string>());
    } catch (const std::invalid_argument &e) {
      throw client_error(400,
                         std::string("input_jpeg is not valid base64: ") + e.what());
    }

    cv::Mat image = image_ops::decode(jpeg_bytes);
    if (image.empty())
      throw client_error(400, "input_jpeg is not a decodable image");

    cv::Mat resized = image_ops::resize(image, width, height);
    std::vector<std::uint8_t> output_bytes = image_ops::encode_jpeg(resized);

    json response = {{"code", 200},
                     {"message", "success"},
                     {"output_jpeg", base64::encode(output_bytes)}};
    return {200, response.dump()};
  } catch (const client_error &e) {
    return error_response(e.status, e.what());
  } catch (const std::exception &e) {
    // unexpected internal failure
    return error_response(500, std::string("internal error: ") + e.what());
  }
}
}  // namespace resize_service
