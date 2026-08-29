# Service Hub

Service Hub is the addressable request/response service of Dispatcher.

## Current implementation stage

`CORE-002 / Step 1` creates only the independent service skeleton.

Implemented in this step:

- separate `services/service-hub/` source tree;
- separate C++20 core library;
- separate `dispatcher-service-hub` executable;
- separate `dispatcher-service-hub-tests` test executable;
- CTest registration under the `service-hub.` prefix;
- integration into the root CMake project.

Not implemented yet:

- transport;
- serialization;
- provider registration;
- routing;
- request/response envelopes;
- correlation;
- Web-facing access;
- authentication or authorization.

Those decisions intentionally remain outside Step 1. Transport and the minimal external contract are selected in `CORE-002 / Step 2`.

## Build in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_service_hub dispatcher_service_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^service-hub\."

## Run

    "$HOME/.cache/dispatcher/build/debug/services/service-hub/dispatcher-service-hub"

Expected output:

    Dispatcher Service Hub

At this stage the executable exits immediately after confirming that the service skeleton starts. A long-running transport lifecycle is added only after the transport is selected.
