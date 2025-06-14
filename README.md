# Secure Tokenizer & Encoder (C++)

A high-performance, reversible token encoder and pseudonymizer. This system provides secure encoding mechanisms using both AES-256-ECB and HMAC/BLAKE3-based token transformations.

---

## ✨ Features

- 🔒 AES-256 ECB reversible encryption with PKCS7 padding
- 🔁 Deterministic token pseudonymization using HMAC or BLAKE3
- 🚀 Fast Base64 encoding/decoding via AVX2-optimized backend
- ⚖️ Bias-free and structure-preserving token mappings
- 🔧 Exposes HTTP endpoints for encode/decode via uWebSockets
- 📊 Catch2-based test coverage for both correctness and performance

---

## 📦 Project Structure

```
src/
  AES256ECB.cpp/hpp       # AES-256-ECB encoding/decoding
  Base64.cpp/hpp          # Fast Base64 via fastavxbase64
  tokenizer.cpp/hpp       # HMAC and BLAKE3 pseudonymizers
  Curl.hpp                # (not currently used)
  CircularPool.hpp        # (not currently used)
  main.cpp                # HTTP interface (uWebSockets)

tests/
  test_aes356ecb.cpp     # Deterministic AES validation
  test_http_server.cpp   # Test Performance and accuracy of token encoders

external/
  blake3/                         # Fast cryptographic hash
  catch2/                         # Testing framework
  fastbase64/                     # AVX2 base64
  simdjson/                       # (not currently used)
  uSockets/                       # Socket backend
  uWebSockets/                    # HTTP server
  word/                           # Google's 10,000 english words.
```

---

## 🔐 Usage

### AES-256 ECB

```
POST /encode/aes256ecb
POST /decode/aes256ecb
```

Input: raw token in body (first line).  
Output: encoded or decoded token as Base64.

## 🧪 Testing

### Build and Test

```bash
mkdir build && cd build
cmake ..
cmake --build . -- -j
```

Run unit and performance tests:

```bash
./test_aes356ecb
./test_tokenizer_performance -s
```

---

## ⚡ Benchmark (WSL2, Ryzen 5800X)

```
[encode] 10000 items in 0.551902s = 18119.2 req/s
[decode] 10000 items in 0.527206s = 18967.9 req/s
```

---

## 🛠 Requirements

- CMake ≥ 3.20
- OpenSSL development libraries
- C++20-compatible compiler (GCC 11+, Clang 13+)
- AVX2-capable CPU for fastbase64

---

## 📄 License

© Micho Todorovich. All rights reserved.
