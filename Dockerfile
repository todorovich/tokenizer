# ---------- Stage 1: Build ----------
FROM alpine:3.22 AS builder

RUN apk add --no-cache \
    build-base \
    cmake \
    curl-dev \
    git \
    openssl-dev

WORKDIR /server
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --target http_server -- -j$(nproc)

# ---------- Stage 2: Runtime ----------
FROM alpine:3.22

RUN apk add --no-cache libstdc++

WORKDIR /server
# Copy just the server binary (and any other required assets/config)
COPY --from=builder /server/build/http_server .

EXPOSE 8080
CMD ["./http_server"]
