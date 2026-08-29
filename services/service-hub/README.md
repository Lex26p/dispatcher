# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation stage

`CORE-002 / Step 4` adds the first real WebSocket request/response route.

The external v1 contract remains the WebSocket + UTF-8 JSON protocol fixed in Step 2, and the provider routing rules remain those implemented in Step 3.

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
- client requests over a separate real WebSocket connection;
- routing by `service`;
- Hub-generated provider request IDs (`hub-*`);
- restoration of the original client request ID in the returned response;
- opaque JSON payload forwarding;
- success and provider error response forwarding;
- basic unknown-service/invalid-request handling required by the route;
- bounded request waiting using the v1 `timeout_ms` value.

The network test uses independent provider and client WebSocket connections against a real loopback TCP listener.

## Internal JSON implementation

The external protocol is JSON and does not depend on a C++ JSON library.

The current C++ implementation uses `json-c` internally for parsing and serialization.

This is not part of the Service Hub v1 contract and can be changed later without changing clients/providers.

## Step 4 routing model

The current successful path is:

    Client WebSocket
      -> Service Hub
      -> ProviderRegistry lookup
      -> Provider WebSocket
      -> provider response
      -> Service Hub
      -> Client WebSocket

The client sends, for example:

    {"type":"request","id":"req-42","service":"test.echo","operation":"echo","payload":{"text":"hello"},"timeout_ms":5000}

The provider receives the same logical request with a Hub-scoped ID such as:

    {"type":"request","id":"hub-1","service":"test.echo","operation":"echo","payload":{"text":"hello"},"timeout_ms":5000}

When the provider responds with `hub-1`, Service Hub returns the response to the client with its original `req-42`.

## Deliberate Step 4 limit

One client session currently processes requests sequentially.

The server already uses Hub-scoped provider IDs and a pending-request table because a real routed response needs correlation, but Step 4 does not claim the full parallel-request behavior required by the v1 contract.

`CORE-002 / Step 5` is responsible for verifying and completing multiple simultaneous requests, including multiple active requests on one client connection.

## Not implemented yet

Step 4 intentionally does not complete:

- full parallel request processing;
- client `cancel`;
- best-effort provider cancel;
- complete provider-disconnect handling for active requests;
- reconnect integration tests;
- browser/Web Shell integration test;
- final lifecycle/signal handling;
- authentication or authorization.

Those remain in the following CORE-002 steps.

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
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_service_hub dispatcher_service_hub_tests dispatcher_service_hub_provider_registry_tests dispatcher_service_hub_request_response_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^service-hub\."

Current CTest checks:

- `service-hub.application`;
- `service-hub.provider-registry`;
- `service-hub.request-response`.
