# ---- Build stage ----------------------------------------------------------
# Ubuntu 24.04 ships Boost 1.83 (libasyik needs >=1.81), so apt Boost is
# sufficient and we avoid libasyik's slow Boost-from-source build.
FROM ubuntu:24.04 AS builder

RUN apt-get update && DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends \
        build-essential cmake ca-certificates \
        libboost-all-dev libopencv-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON \
    && cmake --build build -j"$(nproc)"

# Unit tests gate the image build
RUN ctest --test-dir build --output-on-failure

# ---- Runtime stage ---------------------------------------------------------
FROM ubuntu:24.04

# Only the shared libraries the binary needs at runtime
RUN apt-get update && DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends \
        libboost-context1.83.0 libboost-fiber1.83.0 libboost-atomic1.83.0 \
        libboost-date-time1.83.0 libboost-url1.83.0 \
        libopencv-core406t64 libopencv-imgproc406t64 libopencv-imgcodecs406t64 \
        libssl3t64 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/resize_image_server /usr/local/bin/resize_image_server

EXPOSE 8080
CMD ["resize_image_server"]
