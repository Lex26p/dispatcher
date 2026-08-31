# Device Manager

Device Manager is the core backend responsibility for durable device and metric metadata. Runtime metric values remain the responsibility of Data Hub.

## Current implementation stage

`CORE-006 / Step 1` establishes only the C++20 domain model and standalone process skeleton.

Implemented domain baseline:

- stable opaque device and metric IDs represented as bounded non-empty strings;
- Device metadata: `name`, `description`, `location`;
- Metric metadata: optional `device_id`, `name`, `description`, semantic value type, engineering `unit`, `writable`, `working/state` kind and state-metric link;
- standalone metrics are represented by the absence of `device_id`;
- value types map semantically to the existing Data Hub baseline: `bool`, `int64`, `uint64`, `double`, `string`, `bytes`;
- every working metric requires a state-metric link;
- a state metric is read-only and cannot have another state link;
- catalog validation rejects unknown device references, dangling/non-state targets and working/state device-association mismatch;
- the exact runtime state enum/value encoding is intentionally not selected here.

Step 1 does **not** implement persistence, Service Hub operations, authorization, Data Hub calls, driver configuration or Web UI.

## Build and test

From the repository root in Linux/WSL:

```text
cmake -S . -B "$HOME/.cache/dispatcher/build/debug" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDISPATCHER_BUILD_TESTS=ON
cmake --build "$HOME/.cache/dispatcher/build/debug" --target dispatcher_device_manager dispatcher_device_manager_domain_tests
ctest --test-dir "$HOME/.cache/dispatcher/build/debug" --output-on-failure -R "^device-manager\\."
```

The executable currently has no configuration arguments:

```text
dispatcher-device-manager
```

It only proves an independent Linux process boundary with clean SIGINT/SIGTERM lifecycle. External service/provider behavior belongs to later CORE-006 steps.
