# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 3` adds the internal current-value store.

Implemented at this stage:

- independent C++ service skeleton;
- Protocol Buffers contract in `proto/dispatcher/data_hub/v1/data_hub.proto`;
- gRPC service definition;
- C++ protobuf/gRPC code generation during the build;
- internal thread-safe current-value storage;
- replacement of the previous current sample when the same metric is published again;
- tests for contract serialization and current-value storage.

Real RPC handlers, subscriptions, state-metric behavior and write routing are intentionally implemented in later steps of `CORE-001`.

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

The current build uses distribution packages instead of building gRPC and Protocol Buffers from source.

Install:

    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build pkg-config protobuf-compiler libprotobuf-dev protobuf-compiler-grpc libgrpc++-dev

## Build in WSL

The recommended build directory is stored on the WSL filesystem instead of `/mnt/c`.

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_data_hub dispatcher_data_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^data-hub\."
    "$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub"

## Current-value storage

`CurrentValueStore` is an internal Data Hub component.

Its responsibility is deliberately narrow:

- keep one current `MetricSample` per metric id;
- replace the previous current sample on a new successful `put`;
- return the current sample by metric id;
- keep different metrics independent.

The store does not keep historical samples.

The store rejects samples that have no non-empty metric id or no concrete `MetricValue`.

The store is thread-safe because future gRPC handlers can access the same runtime state concurrently. No network behavior is implemented in the store itself.

## Contract boundary

The Data Hub protobuf package is versioned as:

    dispatcher.data_hub.v1

The initial RPC surface remains:

- `PublishMetric`;
- `GetCurrent`;
- `Subscribe`;
- `WriteMetric`.

The contract does not contain device descriptions, units, project membership or access rights. Those responsibilities belong to other services.
