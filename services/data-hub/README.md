# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 4` connects the current-value store to the real gRPC service boundary.

Implemented at this stage:

- independent C++ Data Hub executable;
- Protocol Buffers / gRPC contract;
- internal thread-safe current-value storage;
- working `PublishMetric` RPC;
- working `GetCurrent` RPC;
- gRPC server listening on a configurable address;
- transport-level tests using two independent gRPC client channels.

`Subscribe` and `WriteMetric` remain explicitly unimplemented until their planned steps.

## Toolchain baseline

- Linux is the target backend environment.
- Local backend development uses WSL.
- C++ standard: C++20.
- Build system: CMake 3.20 or newer.
- Preferred local generator: Ninja.
- Tests are exposed through CTest.
- Data Hub transport: gRPC.
- Data Hub serialization/schema: Protocol Buffers proto3.

## Ubuntu / WSL dependencies

    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build pkg-config protobuf-compiler libprotobuf-dev protobuf-compiler-grpc libgrpc++-dev

## Build in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_data_hub dispatcher_data_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^data-hub\."

## Run

Default endpoint:

    0.0.0.0:50051

Run with the default endpoint:

    "$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub"

Run on a custom endpoint:

    "$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub" 127.0.0.1:50052

The process blocks while serving gRPC requests. Graceful OS-signal handling is intentionally left for `CORE-001 / Step 8`.

## Step 4 RPC behavior

### PublishMetric

A valid sample is stored as the current value for its metric id.

Invalid samples return gRPC `INVALID_ARGUMENT`.

### GetCurrent

Returns the last stored sample for the requested metric id.

An unknown metric returns gRPC `NOT_FOUND`.

### Subscribe

Returns gRPC `UNIMPLEMENTED` until `CORE-001 / Step 5`.

### WriteMetric

Returns gRPC `UNIMPLEMENTED` until `CORE-001 / Step 7`.

## Test boundary

The Step 4 test starts Data Hub on `127.0.0.1:0`, allowing gRPC to choose a free local TCP port.

It then creates two independent gRPC channels:

- a publisher client;
- a reader client.

The publisher sends `PublishMetric`, and the reader retrieves the same sample through `GetCurrent`.

This checks the real gRPC/TCP service boundary while avoiding a fixed test port.
