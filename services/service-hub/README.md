# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation stage

`CORE-002 / Step 3` adds the in-process provider registry and routing table.

The external v1 contract remains the WebSocket + UTF-8 JSON protocol fixed in Step 2.

## Implemented

Current implementation includes:

- independent C++20 Service Hub skeleton;
- external Service Hub v1 contract and JSON Schema;
- `ProviderRegistry`;
- service-address validation matching the v1 contract;
- one active provider connection per service address;
- one registered service per provider connection;
- exact `service -> connection_id` lookup;
- reverse `connection_id -> service` lookup;
- route removal when a provider connection is removed;
- re-registration of the same service by a new connection after disconnect;
- rejection of registration conflicts.

`ProviderRegistry` is thread-safe and does not depend on WebSocket/Boost session types. The future transport layer gives it an internal `ProviderConnectionId`.

## Registration rules

A valid v1 service address:

- is 1..128 characters long;
- starts with lowercase ASCII letter or digit;
- otherwise contains only lowercase ASCII letters, digits, `.`, `_`, `-`.

Registration results distinguish:

- successful registration;
- invalid service address;
- service already owned by another provider;
- connection already registered for a service.

A second registration never silently replaces an active route.

## Contract

Architecture document:

`docs/architecture/service-hub-contract.md`

Machine-readable schema:

`services/service-hub/protocol/dispatcher/service_hub/v1/service_hub.schema.json`

## Not implemented yet

Step 3 intentionally does not implement:

- WebSocket server;
- JSON message parsing;
- network provider sessions;
- forwarding client requests to providers;
- request correlation state;
- timeout timers;
- cancellation;
- Web client integration;
- authentication or authorization.

The real transport path starts in `CORE-002 / Step 4`.

## Build and test in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_service_hub dispatcher_service_hub_tests dispatcher_service_hub_provider_registry_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^service-hub\."

Current CTest checks:

- `service-hub.application`;
- `service-hub.provider-registry`.
