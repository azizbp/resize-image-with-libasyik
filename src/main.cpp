#include "libasyik/http.hpp"
#include "libasyik/service.hpp"

#include "resize_handler.hpp"

int main()
{
  auto as = asyik::make_service();
  auto server = asyik::make_http_server(as, "0.0.0.0", 8080);

  // base64 inflates JPEG payloads by ~33%; allow reasonably large bodies
  // (libasyik's default limit is 1MB)
  server->set_request_body_limit(32 * 1024 * 1024);

  server->on_http_request("/resize_image", "POST", [](auto req, auto args) {
    auto result = resize_service::handle_resize_request(req->body);

    req->response.headers.set("content-type", "application/json");
    req->response.body = result.body;
    req->response.result(result.status);
  });

  LOG(INFO) << "resize_image server listening on 0.0.0.0:8080\n";
  as->run();

  return 0;
}
