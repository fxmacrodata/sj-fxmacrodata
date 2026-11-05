# Capabilities

| FXMacroData response form | C/C++ behavior |
| --- | --- |
| Any complete JSON response from public or authenticated operations | Preserve all members and exact bytes; select any value; create a native sj.h reader |
| History, revisions, as-of records, forecasts and nested results | Preserve caller-supplied payload without renaming fields or changing clocks |
| JSON MCP results, errors and discovery/schema documents | Preserve unknown fields and nested tool parameters; no operation or tool allowlist |
| Embedded resource metadata, links and encoded image/audio fields | Retain exact JSON; decoding, downloads and rendering belong to the caller |
| HTTP errors, ordered duplicate headers and non-JSON bodies | Preserve the caller-supplied status, header pairs and raw body |
| SSE/event streams | Retain raw response; caller frames complete events before JSON parsing |
| Requests, authentication, retries, network policy and pagination | Supplied by the customer's existing client |

This is a JSON interoperability companion, not an HTTP SDK. It neither discovers operations over the network nor limits which response documents the caller can supply. Fixture tests use fictional inputs and do not establish live authentication or customer installation.
