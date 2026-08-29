# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 7` adds the basic write-request path.

Implemented at this stage:

- independent C++ Data Hub executable;
- Protocol Buffers / gRPC contract;
- current-value storage;
- `PublishMetric`;
- `GetCurrent`;
- retained/live `Subscribe`;
- generic state-metric behavior;
- `WriteMetric`;
- routing of a write request to the provider registered for the metric.

## Write semantics

`PublishMetric` and `WriteMetric` have deliberately different meanings.

`PublishMetric` reports the factual current runtime value:

    source/provider -> Data Hub -> CurrentValueStore

`WriteMetric` requests that the owner of a metric try to change its value:

    client -> Data Hub -> MetricWriteProvider

A successful `WriteMetric` response means that Data Hub delivered the request to the current provider and the provider accepted it for processing.

It does not mean:

- equipment has already applied the value;
- the physical operation succeeded;
- Data Hub should immediately replace the current value.

The current value changes only after a source/provider later publishes the resulting factual value through `PublishMetric`.

This prevents an operator command from being mistaken for equipment feedback.

## Write routing

`WriteRouter` keeps one current provider for a metric id.

The first implementation uses an internal C++ port:

    MetricWriteProvider

This is intentionally a service-internal abstraction rather than a C++ interface shared between independent services.

`CORE-001` only needs to prove the route from the external `WriteMetric` RPC to a metric owner.

The concrete inter-process protocol used by the future Driver Runtime to become that owner is not invented in this step. It will be defined when Driver Runtime is developed and its real requirements are known.

A future adapter can implement `MetricWriteProvider` and forward the request through the appropriate language-independent service contract.

## Current gRPC results

`WriteMetric` returns:

- `OK` when a provider exists and accepts the request;
- `INVALID_ARGUMENT` for an incomplete write request;
- `NOT_FOUND` when no provider is registered for the metric;
- `FAILED_PRECONDITION` when the provider explicitly rejects the request.

Data Hub does not currently check user permissions or the descriptive writable flag. Those responsibilities are added later through Users & Access, Service Hub and Device Manager.

## State metrics

State metrics continue to use the same generic publish/get/subscribe path as all other metrics.

No state-specific storage or RPC exists.

## Toolchain

- Linux / WSL;
- C++20;
- CMake 3.20+;
- Ninja;
- gRPC;
- Protocol Buffers proto3;
- CTest.

## Build and test in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_data_hub dispatcher_data_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^data-hub\."

The Step 7 test verifies:

1. a test provider is registered for `AHU01.Setpoint`;
2. the last factual value `22.0` is published;
3. a gRPC client sends `WriteMetric(Setpoint, 24.0)`;
4. the test provider receives exactly that request;
5. `GetCurrent(Setpoint)` still returns `22.0`;
6. an unowned metric returns `NOT_FOUND`;
7. an invalid write returns `INVALID_ARGUMENT`;
8. provider rejection returns `FAILED_PRECONDITION`.
