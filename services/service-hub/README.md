# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation status

`CORE-002 — Service Hub` is complete after sprint acceptance and documentation review.

The external Service Hub v1 WebSocket + UTF-8 JSON contract is implemented and verified by integration tests.

## Implemented

Current implementation includes:

- independent C++20 Service Hub process;
- WebSocket endpoint `/v1/ws`;
- subprotocol `dispatcher.service-hub.v1`;
- provider registration and routing by `service`;
- parallel request correlation;
- client-local request IDs and Hub-scoped provider IDs;
- timeout handling;
- direct browser-compatible client boundary;
- client `cancel`;
- best-effort provider `cancel` after client cancellation, client disconnect or timeout;
- `hub.provider_unavailable` for active requests when a provider disconnects;
- provider route removal and re-registration after reconnect;
- ignored late provider responses for already timed-out/cancelled requests;
- bounded server shutdown with active connections;
- real process shutdown on SIGINT and SIGTERM;
- basic lifecycle diagnostics.

## Lifecycle executable

The executable accepts an optional listen address:

    dispatcher-service-hub [listen-address]

Default:

    0.0.0.0:50052

Example for an ephemeral loopback port:

    dispatcher-service-hub 127.0.0.1:0

On successful start it reports:

    Dispatcher Service Hub listening on <listen-address> (bound port <port>)

SIGINT and SIGTERM are blocked before Service Hub worker threads are created and are synchronously consumed by the application thread through `sigwait()`.

Shutdown diagnostics include the signal name and final stopped message.

## Error and reconnect behavior

The Step 7 integration test verifies:

- unknown service -> `hub.unknown_service`;
- invalid request -> `hub.invalid_request`;
- request timeout -> `hub.timeout`;
- timeout sends `cancel` to the provider;
- a late provider response after timeout is ignored and the provider connection remains usable;
- client `cancel` -> `hub.cancelled` and provider receives `cancel`;
- a client connection remains usable after cancelling one request;
- provider disconnect during an active request -> `hub.provider_unavailable`;
- the disconnected provider route is removed;
- a new provider connection can register the same service and routing works again;
- client disconnect sends provider cancellation for active work;
- `ServiceHubServer::shutdown()` remains bounded with an active long-running request.

## Browser boundary

The future Web Shell continues to use the same direct WebSocket boundary:

    const socket = new WebSocket(
      serviceHubUrl,
      "dispatcher.service-hub.v1"
    );

No additional browser gateway is introduced in Step 7.

## Internal implementation notes

Boost.Asio + Boost.Beast provide the WebSocket/networking layer.

`json-c` is used internally for JSON parsing/serialization and is not part of the public protocol.

Recent timed-out/cancelled Hub request IDs are retained in a bounded internal set so a late provider response can be ignored as required by the v1 contract instead of being mistaken for an unknown request.

## Sprint acceptance

`service-hub.sprint-acceptance` verifies one end-to-end scenario through real loopback WebSocket connections:

- provider registration;
- browser-shaped client connection;
- successful request/response;
- parallel requests with reverse-order provider responses;
- unknown-service error;
- provider disconnect during an active request;
- provider reconnect and route recovery;
- bounded shutdown with an active request.

## Known limitations after CORE-002

- no authentication or authorization;
- no production Origin/TLS policy;
- in-memory provider registry only;
- one active provider per service and no load balancing;
- no clustering/high availability;
- no production-final backpressure/queue policy;
- no active application-defined heartbeat interval;
- no production observability/log aggregation;
- the real React Web Shell starts in `CORE-003`.

## Dependencies

On Ubuntu/WSL the Service Hub development dependencies include:

    libboost-dev
    libjson-c-dev

## Build and test in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_service_hub dispatcher_service_hub_tests dispatcher_service_hub_provider_registry_tests dispatcher_service_hub_request_response_tests dispatcher_service_hub_browser_boundary_tests dispatcher_service_hub_lifecycle_tests dispatcher_service_hub_sprint_acceptance
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^service-hub\."

Current CTest checks:

- `service-hub.application`;
- `service-hub.provider-registry`;
- `service-hub.request-response`;
- `service-hub.browser-boundary`;
- `service-hub.lifecycle-and-errors`;
- `service-hub.signal-term`;
- `service-hub.signal-int`;
- `service-hub.sprint-acceptance`.
