# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation stage

`CORE-002 / Step 6` confirms the direct browser-facing boundary required by the future Web Shell.

The external v1 WebSocket + UTF-8 JSON contract remains unchanged. No HTTP/gRPC-Web gateway is added.

## Implemented

Current implementation includes:

- independent C++20 Service Hub skeleton;
- external Service Hub v1 contract and JSON Schema;
- thread-safe `ProviderRegistry`;
- Boost.Asio + Boost.Beast WebSocket transport;
- endpoint `/v1/ws`;
- required WebSocket subprotocol `dispatcher.service-hub.v1`;
- UTF-8 JSON text messages;
- provider registration over a real WebSocket connection;
- request/response routing through independent client/provider connections;
- parallel request correlation and out-of-order responses;
- client-local request ID namespaces;
- timeout handling through one shared deadline monitor;
- direct browser-compatible WebSocket entry point verified by a dedicated integration test.

## Browser/Web Shell boundary

The future Web Shell connects directly with the standard browser WebSocket API:

    const socket = new WebSocket(
      "ws://host:port/v1/ws",
      "dispatcher.service-hub.v1"
    );

No custom HTTP request headers are required by CORE-002.

The Step 6 integration test uses a browser-shaped handshake:

- standard WebSocket Upgrade request;
- `Origin` header like a development Web application;
- `Sec-WebSocket-Protocol: dispatcher.service-hub.v1`;
- no custom authentication or application HTTP headers;
- normal masked WebSocket client frames;
- UTF-8 JSON application messages.

The test verifies that Service Hub:

1. accepts the connection;
2. returns `101 Switching Protocols`;
3. explicitly negotiates `dispatcher.service-hub.v1`;
4. accepts a JSON request from that connection;
5. routes it through a registered provider;
6. returns the response with the original browser-client request ID.

This proves the concrete transport boundary required by `CORE-003 — Web Shell` without creating the React application early.

## Security note

CORE-002 does not define production Origin policy, authentication tokens or TLS.

A development-style `Origin` is accepted because those policies belong to future security/deployment work. The application protocol does not require browser-incompatible custom headers.

Production deployment may use `wss://` and a reverse proxy without changing the Service Hub v1 application messages.

## Internal JSON implementation

The external protocol is JSON and does not depend on a C++ JSON library.

The current implementation uses `json-c` internally for parsing and serialization. This remains an implementation detail.

## Not implemented yet

Step 6 intentionally does not complete:

- client `cancel`;
- best-effort provider cancel;
- complete provider-disconnect handling for active requests;
- provider reconnect integration tests;
- final lifecycle/signal handling;
- authentication or authorization;
- production Origin/TLS policy;
- the React Web Shell itself.

The next step is `CORE-002 / Step 7 — Lifecycle, ошибки и переподключение`.

## Dependencies

In addition to the existing C++ toolchain, Service Hub transport currently needs:

- Boost headers with Boost.Asio/Boost.Beast;
- `json-c` development files;
- pthread support through CMake `Threads`.

On Ubuntu/WSL the development packages are typically:

    libboost-dev
    libjson-c-dev

## Build and test in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_service_hub dispatcher_service_hub_tests dispatcher_service_hub_provider_registry_tests dispatcher_service_hub_request_response_tests dispatcher_service_hub_browser_boundary_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^service-hub\."

Current CTest checks:

- `service-hub.application`;
- `service-hub.provider-registry`;
- `service-hub.request-response`;
- `service-hub.browser-boundary`.
