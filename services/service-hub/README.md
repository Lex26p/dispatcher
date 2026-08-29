# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation stage

`CORE-002 / Step 2` fixes the external transport and message contract.

The independent C++ skeleton from Step 1 remains unchanged in this step.

## Selected transport

Service Hub v1 uses:

- WebSocket;
- UTF-8 JSON text messages;
- endpoint path `/v1/ws`;
- WebSocket subprotocol `dispatcher.service-hub.v1`;
- machine-readable JSON Schema for the generic envelope.

The same client protocol is usable by C++ backend clients and by the future browser Web Shell.

Provider connections are also WebSocket connections. A provider registers one service address and then receives routed requests over that persistent connection.

## Contract

Architecture document:

`docs/architecture/service-hub-contract.md`

Machine-readable schema:

`services/service-hub/protocol/dispatcher/service_hub/v1/service_hub.schema.json`

The generic envelope defines:

- provider registration;
- service and operation addressing;
- request/response correlation;
- success/error responses;
- timeout;
- cancellation;
- disconnect/reconnect semantics;
- protocol errors and message limits.

Business payload remains opaque JSON to Service Hub. Each future provider owns the schema and meaning of its operations.

## Why not gRPC for Service Hub

Data Hub continues to use gRPC + Protocol Buffers.

Service Hub has different requirements: its provider side needs a persistent reverse request channel while the client side must be directly browser-compatible for the next Web Shell sprint.

Using WebSocket for both roles avoids adding a mandatory gRPC-Web proxy/gateway only to make Service Hub reachable from the browser.

## C++ implementation

The v1 contract does not depend on a C++ networking library.

The planned C++ implementation uses Boost.Asio + Boost.Beast for WebSocket/networking.

The concrete JSON parsing library is an internal implementation choice and is not part of the external protocol.

## Not implemented yet

Step 2 does not yet implement:

- WebSocket server;
- provider registration table;
- routing;
- request correlation state;
- timeout timers;
- cancellation;
- reconnect behavior;
- Web client integration;
- authentication or authorization.

Those are implemented and tested in the following CORE-002 steps.

## Existing Step 1 build

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_service_hub dispatcher_service_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^service-hub\."

Step 2 changes documentation and protocol schema only, so rebuilding the unchanged Step 1 executable is not required for this step.
