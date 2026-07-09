#pragma once

#include <string>

namespace resize_service
{
// Result of handling one /resize_image request, ready to be written to the
// HTTP response.
struct handler_result {
  int status;        // HTTP status code
  std::string body;  // JSON payload
};

// Full request flow: parse JSON -> validate -> base64-decode -> imdecode ->
// resize -> imencode -> base64-encode. Never throws; every error is mapped
// to the JSON error
handler_result handle_resize_request(const std::string &request_body);
}  // namespace resize_service
