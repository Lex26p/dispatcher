# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 5` implements subscriptions.

Implemented at this stage:

- independent C++ Data Hub executable;
- Protocol Buffers / gRPC contract;
- internal thread-safe current-value storage;
- working `PublishMetric` RPC;
- working `GetCurrent` RPC;
- working server-streaming `Subscribe` RPC;
- retained-like delivery of an already existing current value;
- live delivery of later changes for explicitly subscribed metric ids;
- isolation between subscriptions and unrelated metric updates.

`WriteMetric` remains explicitly unimplemented until `CORE-001 / Step 7`.

## Subscription behavior

The v1 contract subscribes to an explicit list of metric ids.

An empty list is invalid and does not mean "subscribe to everything".

For each requested metric that already has a current value, the new subscriber receives that value first.

After retained/current values are queued, later `PublishMetric` calls for those metric ids are delivered as live `MetricUpdate` messages.

Updates for metric ids that are not part of the subscription are not delivered.

The registration of a subscription is coordinated with publication so that a publish operation cannot occur between registering the subscriber and queuing its retained values. This prevents losing an update at the retained/live boundary.

## Internal implementation

`SubscriptionManager` tracks active subscriptions.

Each `Subscription` has:

- a set of metric ids;
- its own pending-update queue;
- a condition variable used by the synchronous gRPC streaming handler.

Publishing does not write to a client socket directly. It queues the sample for matching subscribers. The individual `Subscribe` RPC handler owns its `ServerWriter` and sends queued updates from its own call thread.

This keeps concurrent publishers from writing to the same gRPC stream.

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

The Step 5 transport test:

1. publishes `Temperature = 25`;
2. starts a subscription to `Temperature`;
3. verifies that the subscriber immediately receives `25`;
4. publishes an unrelated `Pressure` value;
5. publishes `Temperature = 26`;
6. verifies that the next subscription update is `Temperature = 26`;
7. cancels the stream and verifies normal cancellation behavior.
