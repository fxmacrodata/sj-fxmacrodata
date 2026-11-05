# FXMacroData for sj.h

C++17 response ownership and JSON selection for applications using rxi's C99 sj.h tokenizer. Keep an FXMacroData response intact, select any JSON value by RFC 6901 pointer, and obtain an independently owned native sj.h reader for that value.

## Use

Supply the original `sj.h` from `rxi/sj.h`, commit `5cb5df45c8c37fd8c2322026a111603e2adfa675` (v0.4). The upstream header is independently supplied and retains its upstream public-domain terms. Compile `sj_impl.c` as C99 and `fxmacrodata_sj.cpp` as C++17, with both this directory and the upstream header directory on the include path. The CMake project accepts `SJ_INCLUDE_DIR` and provides a fixture-only test target; it never downloads a dependency.

```cpp
#include "fxmacrodata_sj.hpp"
// Pass status, ordered header pairs and complete body from your HTTP client.
fxmd_sj::Response reply{200, {{"Content-Type", "application/json"}},
    R"({"currency":"USD","value":12345678901234567890.00100,"extra":[null,true]})"};
auto json = reply.json();
auto exact_number = json.at("/value").raw();
auto native = json.at("/extra").native();
sj_Value item;
while (sj_iter_array(&native->reader(), native->root(), &item)) {
    // Consume the native token here; native owns its backing bytes.
}
```

The example is fictional. Your HTTP or FXMacroData client supplies the response. This library does not make requests, handle credentials, implement SSE, render media or change FXMacroData access entitlements. Public USD access and optional authenticated coverage follow your client's normal FXMacroData contract.

## Fidelity and lifetime

`Response` retains status, ordered duplicate header pairs and every body byte, including unsuccessful responses and non-JSON bodies. Parsing creates a separate document; a rejected parse leaves the response untouched. `Document::bytes()` retains whitespace and complete original JSON. `Value::raw()` preserves the selected value's exact JSON text, including number spelling, nulls, unknown fields and clock strings. No conversion to floating point takes place. `Value::text()` decodes a string, including surrogate pairs and embedded NUL bytes.

`Document::at()` selects the root with an empty pointer, supports `~0` and `~1`, and uses exact decoded object keys. Array indices are zero based and have no leading zeros. Missing members raise `std::out_of_range`; invalid JSON/pointer syntax raises `std::invalid_argument`. Selected values retain shared ownership even after their document is destroyed.

`Value::native()` copies just that complete selected value into a noncopyable `NativeSnapshot`. Its reader and tokens remain valid while that snapshot exists. Do not retain native pointers after destroying it, replace reader pointers, or mutate its backing memory. Only container-opening tokens have upstream depth semantics; the snapshot's scalar root depth is normalized to zero. Native iteration uses sj.h's original API.

## JSON boundaries

Strict parsing requires one complete UTF-8 JSON value, valid number grammar, matching delimiters and valid escapes/surrogates. Duplicate decoded object keys are rejected. Limits are 4 MiB per JSON document, nesting depth 128 and 100,000 values. Raw response retention is independent of those parsing limits. Binary media bodies and event streams remain raw until a caller extracts an individual complete JSON document.

See `CAPABILITIES.md` for supported response forms and transport boundaries. This companion is independently maintained and is not an upstream sj.h endorsement.
