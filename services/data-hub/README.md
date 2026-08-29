# Data Hub

Data Hub is the runtime service responsible for current metric values in Dispatcher.

## Current implementation stage

`CORE-001 / Step 8` completes the basic service lifecycle and error-handling work planned for the sprint.

Implemented at this stage:

- current-value storage;
- `PublishMetric`;
- `GetCurrent`;
- retained/live `Subscribe`;
- generic state metrics;
- `WriteMetric` routing to a metric provider;
- bounded graceful gRPC shutdown;
- SIGINT and SIGTERM handling for the Linux service process;
- client disconnect/reconnect verification;
- subscription cleanup and shutdown verification;
- baseline gRPC status handling for invalid and unknown requests;
- concise startup/shutdown diagnostics.

## Process lifecycle

The Linux process blocks SIGINT and SIGTERM before the gRPC server creates worker threads.

The main application thread then waits synchronously for one of those shutdown signals.

On SIGINT or SIGTERM:

1. Data Hub reports the shutdown request;
2. gRPC stops accepting new calls;
3. active RPCs receive a two-second graceful completion window;
4. calls that are still active after that deadline are cancelled;
5. the server waits for gRPC worker activity to stop;
6. the process reports that Data Hub stopped and exits with status 0.

This bounded shutdown is important for long-lived `Subscribe` streams: an abandoned or slow client must not prevent the service process from stopping indefinitely.

## Diagnostics

Step 8 deliberately does not introduce a logging framework.

The executable currently reports only lifecycle-level messages:

- startup/listening endpoint and actual bound port;
- failure to configure shutdown signals;
- failure to start the server;
- received shutdown signal;
- clean stop.

Detailed production logging and monitoring can be introduced later when there are real requirements for them.

## Error behavior

Current gRPC behavior includes:

- invalid `PublishMetric` -> `INVALID_ARGUMENT`;
- unknown `GetCurrent` -> `NOT_FOUND`;
- empty `Subscribe` -> `INVALID_ARGUMENT`;
- invalid `WriteMetric` -> `INVALID_ARGUMENT`;
- write without a provider -> `NOT_FOUND`;
- provider rejection -> `FAILED_PRECONDITION`;
- cancelled subscription -> non-success cancellation status.

## Reconnection

Current runtime values belong to the running Data Hub process, not to a client connection.

A client may disconnect and create a new gRPC channel. The new client can still:

- retrieve the last current value;
- create a new subscription;
- receive the retained current value.

This does not imply persistence across a Data Hub process restart. Runtime-state recovery remains outside the current sprint.

## Build and test in WSL

    cd /mnt/c/Projects/dispatcher
    cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
    cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_data_hub dispatcher_data_hub_tests
    ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^data-hub\."

Step 8 adds three CTest checks:

- the C++ lifecycle/error/reconnect test;
- real process shutdown through SIGTERM;
- real process shutdown through SIGINT.

## Run manually

Default endpoint:

    "$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub"

Custom endpoint:

    "$HOME/.cache/dispatcher/build/debug/services/data-hub/dispatcher-data-hub" 127.0.0.1:50052

Ctrl+C sends SIGINT and should result in lifecycle output ending with:

    Dispatcher Data Hub shutdown requested by SIGINT
    Dispatcher Data Hub stopped

The automated CTest signal checks already verify this behavior; a separate manual check is not required for Step 8.
