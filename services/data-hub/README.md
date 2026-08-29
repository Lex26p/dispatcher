# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 1` creates only the independent C++ service skeleton and its test target.

Metric contracts, storage, subscriptions, state metrics and write routing are intentionally implemented in later steps of `CORE-001`.

## Toolchain baseline

- Linux is the target backend environment.
- Local backend development uses WSL.
- C++ standard: C++20.
- Build system: CMake 3.20 or newer.
- Preferred local generator: Ninja.
- Tests are exposed through CTest.

No external C++ libraries are required by Step 1.

## Build in WSL

The recommended build directory is stored on the WSL filesystem instead of `/mnt/c` to avoid unnecessary filesystem overhead while compiling C++.

```sh
cd /mnt/c/Projects/dispatcher
cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_data_hub dispatcher_data_hub_tests
ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R '^data-hub\.'
"$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub"
```
