# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation stage

`CORE-002 / Step 5` completes the first parallel request-correlation model.

The external v1 WebSocket + UTF-8 JSON contract remains unchanged.

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
- client requests over independent WebSocket connections;
- routing by `service`;
- Hub-generated provider request IDs (`hub-*`);
- restoration of the original client request ID in returned responses;
- opaque JSON payload forwarding;
- success and provider error response forwarding;
- multiple simultaneously active requests on one client connection;
- out-of-order provider responses without response mix-ups;
- independent client request-ID namespaces per client connection;
- a global pending-correlation table instead of one worker thread per request;
- one timeout monitor for active request deadlines;
- basic unknown-service/invalid-request handling required by the route.

## Correlation model

A client request ID is unique only among active requests of that client connection.

Service Hub creates a separate provider-scoped ID:

    client connection A + req-42 -> hub-1
    client connection B + req-42 -> hub-2

The provider therefore never sees conflicting client-local IDs.

When a provider response arrives, Service Hub:

1. resolves its `hub-*` ID in the global pending table;
2. identifies the original client session;
3. restores the original client request ID;
4. queues the response to that client session.

Each client session owns its WebSocket reads and writes. Completion from another thread does not write directly to the WebSocket; it only appends to the session outbound queue. This keeps WebSocket I/O serialized.

## Parallel behavior verified in Step 5

The network test now covers:

1. normal single request/response routing;
2. two requests active at the same time on one client connection;
3. provider replies in reverse order and each response returns to the correct request ID;
4. one request reaches `hub.timeout` while another request on the same connection succeeds first;
5. two different client connections use the same client request ID simultaneously and receive their own responses;
6. provider-scoped IDs remain different in that case.

This is the required Step 5 correlation behavior.

## Timeout model

Requests are stored in one global pending table.

A single timeout-monitor thread expires armed entries and queues `hub.timeout` back to the corresponding client. Service Hub does not create one operating-system thread per request.

Provider-side best-effort `cancel` for expired requests is still deferred to Step 7 as planned.

## Internal JSON implementation

The external protocol is JSON and does not depend on a C++ JSON library.

The current implementation uses `json-c` internally for parsing and serialization. This remains an implementation detail.

## Not implemented yet

Step 5 intentionally does not complete:

- client `cancel`;
- best-effort provider cancel;
- complete provider-disconnect handling for active requests;
- provider reconnect integration tests;
- browser/Web Shell integration test;
- final lifecycle/signal handling;
- authentication or authorization.

The next step is `CORE-002 / Step 6 — Клиентская граница для Web Shell`.

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

The last test includes both the basic route and the Step 5 parallel-correlation scenarios.
