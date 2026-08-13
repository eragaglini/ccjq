# ccjq — Coding Challenges jq

A lightweight C command-line JSON processor and parser inspired by [`jq`](https://jqlang.github.io/jq/).  
Built as part of the [Coding Challenges - Build Your Own jq](https://codingchallenges.fyi/challenges/challenge-jq).

---

## 🚀 Features

- **Standard JSON Parser:** Compliant with [RFC 8259](https://datatracker.ietf.org/doc/html/rfc8259) specifications.
- **CLI & Pipe Support:** Reads JSON input directly from files or via `stdin` pipelines.
- **Robust Error Handling:** Detects invalid JSON syntax, unclosed brackets/quotes, trailing commas, and invalid numbers.
- **C11 / C99 Compliant:** Written in portable, clean C without external dependencies.

---

## 🛠️ Building

The project uses a standard `Makefile` with targets for both optimized Release builds and Debug builds (with AddressSanitizer support).

### Standard Build (Release)
Compiles with `-O2` optimizations:
```bash
make
