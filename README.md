# Secure Tokenizer (C++)

A high-performance, cryptographically secure, deterministic tokenizer. It transforms sensitive tokens (e.g., names, IDs) into pseudonymized, fixed-length strings that preserve uniqueness and aggregability, while preventing reversal without a secret key.

---

## ✨ Features

- ✅ **Deterministic**: Same input → same output
- ✅ **HMAC-SHA256-based**: Uses standard cryptographic primitives
- ✅ **Single-core optimized**: Manual HMAC with EVP API
- ✅ **Bias-free character mapping**
- ✅ **All printable ASCII characters supported**
- ✅ **Includes correctness and performance tests**

---

## 🔒 Use Case

Intended for:
- Pseudonymizing structured fields (e.g., names, email handles)
- Maintaining token-level consistency across datasets
- Supporting reversible mapping only with possession of keys

Not suitable for:
- Irreversible one-time encryption
- Cryptographically strong anonymization without controlled keys

---

## 🛠 Build

Requires:
- CMake ≥ 3.20
- OpenSSL (dev headers)
- C++20 compiler

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

To run tests:
```bash
./test_tokenizer
./test_tokenizer_performance -s
```

---

## 🧪 Algorithms

Five implementations (`encode_token1` → `encode_token5`) are provided to compare:

| Version | Notes                                      |
|---------|--------------------------------------------|
| 1       | Baseline double HMAC                       |
| 2       | Single HMAC with SHA-512                   |
| 3       | Optimized string reuse                     |
| 4       | Fixed-key stack buffer                     |
| 5       | Manual HMAC with EVP (fastest)             |

All versions preserve:
- Token length
- Output uniqueness per input
- ASCII-printable character constraints

---

## 🔍 Testing

- `test_tokenizer.cpp`: Correctness tests using Catch2
- `test_tokenizer_performance.cpp`: Performance comparison of all variants
- Generates 1M random tokens and measures throughput

---

## 📦 Example Output

For token `"Fred"` with given key/salt/domain:
```
Encoded: &h4G
```

Output varies depending on key/salt and domain, but is stable per configuration.

---

## 📁 Project Structure

```
src/
  tokenizer.cpp/hpp       # All encoder variants
tests/
  test_tokenizer.cpp      # Functional tests
  test_tokenizer_performance.cpp  # Speed tests
main.cpp                  # (unused placeholder)
```

---

## 🔐 Security Notes

- Keys should be kept secret and unique per dataset/domain.
- If token inputs are low-entropy (e.g. names), preimage attacks are only mitigated by key secrecy.
- Use HMAC-derived output to separate contexts between fields.

---

## 📄 License

© Micho Todorovich. All rights reserved.

