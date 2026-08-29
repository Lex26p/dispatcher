# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 6` verifies and fixes the runtime model for state metrics.

Implemented at this stage:

- independent C++ Data Hub executable;
- Protocol Buffers / gRPC contract;
- internal thread-safe current-value storage;
- working `PublishMetric` RPC;
- working `GetCurrent` RPC;
- working server-streaming `Subscribe` RPC;
- retained-like delivery of current values;
- live delivery of later metric changes;
- working metrics and their state metrics using the same generic Data Hub mechanisms.

`WriteMetric` remains explicitly unimplemented until `CORE-001 / Step 7`.

## State metric model

A state metric is an ordinary metric from the point of view of Data Hub.

Conceptually:

    AHU01.Temperature       = 26.0
    AHU01.Temperature.State = Alarm

The `.State` suffix above is only a readable example used in the current documentation and tests. Data Hub does not parse or enforce that naming convention.

Data Hub does not:

- calculate `Normal`, `Warning`, `Alarm`, `NoData`, `Maintenance`, or any other state;
- define the final set of allowed state values;
- infer which state metric belongs to which working metric;
- store device descriptions or other descriptive metric metadata.

The future Event Manager will calculate the current state and publish the corresponding state metric into Data Hub.

The future Device Manager is the appropriate place for descriptive metadata and the relationship between a working metric and its associated state metric.

Data Hub only stores and distributes the resulting runtime values.

## Subscription behavior

The v1 contract subscribes to an explicit list of metric ids.

For every requested metric that already has a current value, a new subscriber receives that value before later live updates.

A consumer that needs both a working value and its state subscribes to both metric ids.

No special state-specific RPC or subscription type is required.

## Internal implementation

`CurrentValueStore` and `SubscriptionManager` do not distinguish state metrics from other metrics.

That is intentional: the runtime mechanisms remain universal and do not duplicate the domain model.

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

The Step 6 transport test verifies:

1. publication of a working metric;
2. publication of a separate state metric;
3. independent `GetCurrent` for both metrics;
4. retained delivery of both metrics to one subscriber;
5. live delivery of a new working value;
6. live delivery of a new state value.
