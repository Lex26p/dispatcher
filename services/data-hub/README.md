# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 2` adds the first external Data Hub contract.

Implemented at this stage:

- independent C++ service skeleton;
- Protocol Buffers contract in `proto/dispatcher/data_hub/v1/data_hub.proto`;
- gRPC service definition;
- C++ protobuf/gRPC code generation during the build;
- compilation and serialization smoke checks for the contract.

Metric storage, real RPC handlers, subscriptions, state-metric behavior and write routing are intentionally implemented in later steps of `CORE-001`.

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

The current build uses the distribution packages instead of building gRPC and Protocol Buffers from source. This keeps local compilation lighter.

Install:

    sudo apt-get update
    sudo apt-get install -y build-essential cmake ninja-build pkg-config protobuf-compiler libprotobuf-dev protobuf-compiler-grpc libgrpc++-dev

## Build in WSL

The recommended build directory is stored on the WSL filesystem instead of `/mnt/c` to avoid unnecessary filesystem overhead while compiling C++.

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_data_hub dispatcher_data_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^data-hub\."
    "$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub"

## Contract boundary

The Data Hub protobuf package is versioned as:

    dispatcher.data_hub.v1

The initial RPC surface is intentionally small:

- `PublishMetric`;
- `GetCurrent`;
- `Subscribe`;
- `WriteMetric`.

The contract does not contain device descriptions, units, project membership or access rights. Those responsibilities belong to other services.

The active `oneof` branch in `MetricValue` is the wire-level value type. Step 2 supports:

- boolean;
- signed 64-bit integer;
- unsigned 64-bit integer;
- double;
- UTF-8 string;
- bytes.

The contract can evolve by adding new protobuf fields or value alternatives while preserving existing field numbers.
