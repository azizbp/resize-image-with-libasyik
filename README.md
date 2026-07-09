# resize-image-with-libasyik

A small C++17 HTTP microservice that resizes a base64-encoded JPEG to a target
dimension. Built with [libasyik](https://github.com/okyfirmansyah/libasyik)
(Boost.Asio/Beast/Fiber), OpenCV, and nlohmann/json for the **Nodeflux C++
Software Engineer technical assessment (Task 1)**.

## API

The service exposes exactly one endpoint on port **8080**:

```
POST /resize_image
Content-Type: application/json

{
  "input_jpeg":     "<base64 of jpeg binary>",
  "desired_width":  <int>,
  "desired_height": <int>
}
```

**Success — HTTP 200**

```json
{
  "code": 200,
  "message": "success",
  "output_jpeg": "<base64 of output jpeg binary>"
}
```

**Failure — HTTP 4XX / 5XX**

```json
{
  "code": 400,
  "Message": "<error description>"
}
```

Client errors (bad JSON, missing/invalid fields, invalid base64, undecodable
image, non-positive or oversized dimensions) return **400**; unexpected
internal failures return **500**. Responses are always `application/json`.

## Getting the source

Clone **with submodules** (libasyik and its nested submodules are required):

```bash
git clone --recursive <repo-url>
# or, if already cloned:
git submodule update --init --recursive
```

## Build & run with Docker (recommended)

```bash
docker build -t resize-service .
docker run --rm -p 8080:8080 resize-service
```

The Docker build also runs the unit tests; the image fails to build if any
test fails.

### Try it

```bash
BASE64_IMG=$(base64 -w0 input.jpg)   # on macOS: base64 -i input.jpg
curl -s -X POST http://localhost:8080/resize_image \
  -H 'Content-Type: application/json' \
  -d "{\"input_jpeg\":\"$BASE64_IMG\",\"desired_width\":320,\"desired_height\":240}"
```

## Local build (Linux)

Requires CMake ≥3.14, OpenCV (`libopencv-dev`), OpenSSL (`libssl-dev`), and
**Boost ≥1.81** (Ubuntu 24.04's apt Boost 1.83 works; older LTS releases need
Boost built from source — see `third_party/libasyik/Dockerfile` for the
reference build):

```bash
sudo apt-get install -y build-essential cmake libboost-all-dev libopencv-dev libssl-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/resize_image_server
```

## Run tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Project layout

```
src/
  main.cpp            # service bootstrap + endpoint wiring (libasyik)
  resize_handler.*    # request flow: parse -> validate -> resize -> respond
  image_ops.*         # OpenCV decode / resize / encode (no HTTP dependency)
  base64.hpp          # small self-contained base64 codec
tests/
  test_resize.cpp     # Catch2 unit tests (base64, image ops, handler contract)
third_party/
  libasyik/           # git submodule (HTTP framework)
```

## Notes

- libasyik is consumed via `add_subdirectory()`, which also builds libasyik's
  own Catch2 test suite as a side effect — upstream has no flag to skip it.
  This is expected, just extra build time.
- SOCI (database) and SSL-server support in libasyik are disabled
  (`LIBASYIK_ENABLE_SOCI=OFF`, `LIBASYIK_ENABLE_SSL_SERVER=OFF`) since this
  service is a plain-HTTP service with no database.
- The request body limit is raised to 32 MB to accommodate base64-inflated
  JPEG payloads.
- CI (GitHub Actions) builds the Docker image (which runs the tests) and
  smoke-tests the running container on every push/PR.
