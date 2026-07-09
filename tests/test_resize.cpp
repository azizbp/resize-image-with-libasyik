#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <nlohmann/json.hpp>
#include <opencv2/core.hpp>

#include "base64.hpp"
#include "image_ops.hpp"
#include "resize_handler.hpp"

using nlohmann::json;

namespace
{
// A real JPEG generated in-process, so tests need no fixture files.
std::vector<std::uint8_t> make_test_jpeg(int width, int height)
{
  cv::Mat image(height, width, CV_8UC3, cv::Scalar(0, 128, 255));
  return image_ops::encode_jpeg(image);
}
}  // namespace

TEST_CASE("base64 round-trip and known vectors")
{
  std::vector<std::uint8_t> data = {'N', 'o', 'd', 'e', 'f', 'l', 'u', 'x'};
  REQUIRE(base64::decode(base64::encode(data)) == data);

  // RFC 4648 test vectors
  REQUIRE(base64::encode({'M'}) == "TQ==");
  REQUIRE(base64::encode({'M', 'a'}) == "TWE=");
  REQUIRE(base64::encode({'M', 'a', 'n'}) == "TWFu");
}

TEST_CASE("base64 rejects invalid input")
{
  REQUIRE_THROWS_AS(base64::decode(""), std::invalid_argument);
  REQUIRE_THROWS_AS(base64::decode("abc"), std::invalid_argument);   // bad length
  REQUIRE_THROWS_AS(base64::decode("ab!@"), std::invalid_argument);  // bad chars
  REQUIRE_THROWS_AS(base64::decode("a=bc"), std::invalid_argument);  // '=' not trailing
}

TEST_CASE("image_ops decode / resize / encode")
{
  auto jpeg = make_test_jpeg(32, 24);

  cv::Mat image = image_ops::decode(jpeg);
  REQUIRE_FALSE(image.empty());
  REQUIRE(image.cols == 32);
  REQUIRE(image.rows == 24);

  cv::Mat resized = image_ops::resize(image, 10, 5);
  REQUIRE(resized.cols == 10);
  REQUIRE(resized.rows == 5);

  cv::Mat again = image_ops::decode(image_ops::encode_jpeg(resized));
  REQUIRE(again.cols == 10);
  REQUIRE(again.rows == 5);
}

TEST_CASE("image_ops decode rejects non-image bytes")
{
  std::vector<std::uint8_t> garbage = {1, 2, 3, 4, 5};
  REQUIRE(image_ops::decode(garbage).empty());
}

TEST_CASE("handler happy path returns 200 with lowercase message")
{
  json request = {{"input_jpeg", base64::encode(make_test_jpeg(32, 24))},
                  {"desired_width", 100},
                  {"desired_height", 50}};
  auto result = resize_service::handle_resize_request(request.dump());

  REQUIRE(result.status == 200);
  json response = json::parse(result.body);
  REQUIRE(response.at("code") == 200);
  REQUIRE(response.at("message") == "success");  // lowercase on success (§4)

  cv::Mat output = image_ops::decode(
      base64::decode(response.at("output_jpeg").get<std::string>()));
  REQUIRE(output.cols == 100);
  REQUIRE(output.rows == 50);
}

TEST_CASE("handler rejects bad requests with 400 and capital-M Message")
{
  auto valid_jpeg = base64::encode(make_test_jpeg(8, 8));

  struct bad_case {
    const char *name;
    std::string body;
  };
  std::vector<bad_case> cases = {
      {"malformed JSON", "{not json"},
      {"not a JSON object", "[1,2,3]"},
      {"missing input_jpeg",
       json{{"desired_width", 10}, {"desired_height", 10}}.dump()},
      {"non-string input_jpeg",
       json{{"input_jpeg", 5}, {"desired_width", 10}, {"desired_height", 10}}
           .dump()},
      {"missing desired_width",
       json{{"input_jpeg", valid_jpeg}, {"desired_height", 10}}.dump()},
      {"non-integer desired_width",
       json{{"input_jpeg", valid_jpeg},
            {"desired_width", "10"},
            {"desired_height", 10}}
           .dump()},
      {"zero desired_width",
       json{{"input_jpeg", valid_jpeg},
            {"desired_width", 0},
            {"desired_height", 10}}
           .dump()},
      {"negative desired_height",
       json{{"input_jpeg", valid_jpeg},
            {"desired_width", 10},
            {"desired_height", -3}}
           .dump()},
      {"oversized desired_width",
       json{{"input_jpeg", valid_jpeg},
            {"desired_width", 100000},
            {"desired_height", 10}}
           .dump()},
      {"invalid base64",
       json{{"input_jpeg", "!!notbase64!!"},
            {"desired_width", 10},
            {"desired_height", 10}}
           .dump()},
      {"valid base64 but not an image",
       json{{"input_jpeg", base64::encode({1, 2, 3, 4})},
            {"desired_width", 10},
            {"desired_height", 10}}
           .dump()},
  };

  for (const auto &c : cases) {
    INFO(c.name);
    auto result = resize_service::handle_resize_request(c.body);
    REQUIRE(result.status == 400);
    json response = json::parse(result.body);
    REQUIRE(response.at("code") == 400);
    REQUIRE(response.contains("Message"));  // capital M on failure (§4)
    REQUIRE_FALSE(response.contains("message"));
  }
}
