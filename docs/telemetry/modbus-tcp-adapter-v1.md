# Modbus TCP adapter v1

## Summary

Modbus TCP adapter v1 introduces the first concrete telemetry adapter foundation for Dispatcher.

The adapter is implemented as a separate backend library:

```text id="z9o4ie"
backend/libs/dispatcher-modbus
```

This version provides Modbus TCP protocol frame handling, register value decoding, a transport abstraction, polling configuration, a telemetry adapter that performs one polling cycle, and an in-memory simulator transport for protocol smoke tests.

This version does not yet provide a real TCP socket transport and does not yet connect the adapter to `DispatcherRuntime`.

## Library

Main library:

```text id="yb6ulw"
dispatcher-modbus
```

Location:

```text id="c9z7xw"
backend/libs/dispatcher-modbus
```

Main umbrella header:

```cpp id="h2w9rw"
#include <dispatcher/modbus/modbus.hpp>
```

The library currently includes:

```text id="tpzg5s"
ModbusError
ModbusBytes
ModbusFunctionCode
ModbusReadRegistersRequest
ModbusReadRegistersResponse
ModbusExceptionResponse
ModbusFrameCodec
ModbusRegisterValueType
ModbusWordOrder
ModbusDecodedRegisterValue
ModbusRegisterDecoder
ModbusTcpEndpoint
IModbusTcpTransport
ModbusTcpClient
ModbusTagMapping
ModbusPollingConfiguration
ModbusPollRequest
ModbusPollingPlanBuilder
ModbusTelemetrySample
ModbusTelemetryPollError
ModbusTelemetryPollResult
ModbusTelemetryAdapter
ModbusSimulatedTcpTransport
```

## Scope

This version supports:

```text id="ljdz6g"
Modbus TCP MBAP frame encoding
Read Holding Registers request encoding
Read Input Registers request encoding
Read Registers response decoding
Modbus exception response detection
Modbus exception response decoding
uint16 register decoding
int16 register decoding
uint32 register decoding
int32 register decoding
float32 register decoding
high_word_first order
low_word_first order
transport abstraction
client-side response correlation checks
polling configuration validation
read plan generation
single-cycle telemetry polling
in-memory simulator transport
protocol smoke tests without a network server
```

This version intentionally does not yet support:

```text id="j5bqgo"
real TCP socket transport
continuous polling loop
runtime ingest integration
configuration import from JSON/YAML
HTTP API exposure
Modbus coils
Modbus discrete inputs
writes
TLS
authentication
automatic reconnect policy
production timeout handling
```

## ModbusError

Modbus-specific failures are represented by:

```text id="bj8ryq"
dispatcher::modbus::ModbusError
```

Header:

```text id="kqwhnd"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_error.hpp
```

It derives from:

```text id="wxoq13"
std::runtime_error
```

## Types

Core protocol types are defined in:

```text id="l1s8qv"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_types.hpp
```

Main types:

```text id="ucaz9a"
ModbusBytes
ModbusFunctionCode
ModbusReadRegistersRequest
ModbusReadRegistersResponse
ModbusExceptionResponse
```

Supported function codes currently include:

```text id="s9foe8"
0x03 — read_holding_registers
0x04 — read_input_registers
```

The enum also defines:

```text id="yh5tmb"
0x01 — read_coils
0x02 — read_discrete_inputs
```

but this version does not implement coil or discrete input polling.

## Frame codec

`ModbusFrameCodec` handles Modbus TCP frame encoding and decoding.

Header:

```text id="wl0pai"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_frame_codec.hpp
```

Implementation:

```text id="m1fm8w"
backend/libs/dispatcher-modbus/src/modbus_frame_codec.cpp
```

Supported operations:

```text id="dx1wib"
encode_read_registers_request(...)
decode_read_registers_response(...)
is_exception_response(...)
decode_exception_response(...)
```

### Read Holding Registers request

Example logical request:

```text id="lckx8z"
transaction_id = 0x1234
unit_id = 0x11
function_code = 0x03
start_address = 0x006B
quantity = 0x0003
```

Encoded frame:

```text id="p0wwsn"
12 34 00 00 00 06 11 03 00 6B 00 03
```

Frame layout:

```text id="t3l7yh"
transaction_id: 2 bytes
protocol_id: 2 bytes, always 0
length: 2 bytes
unit_id: 1 byte
function_code: 1 byte
start_address: 2 bytes
quantity: 2 bytes
```

### Read Input Registers request

Read Input Registers uses function code:

```text id="h60z24"
0x04
```

The frame structure is the same as Read Holding Registers.

### Read Registers response

A normal register response contains:

```text id="ywm6qt"
transaction_id
protocol_id
length
unit_id
function_code
byte_count
register bytes
```

The codec validates:

```text id="g8yg7r"
minimum frame length
protocol id
MBAP length
supported function code
non-zero byte_count
even byte_count
byte_count matching frame size
```

### Exception response

Modbus exception responses are detected when the high bit of the function code is set:

```text id="yn6yzl"
function_code | 0x80
```

Example exception response:

```text id="ui7n65"
12 34 00 00 00 03 11 83 02
```

This means:

```text id="w10ogm"
transaction_id = 0x1234
unit_id = 0x11
function = 0x03 with exception bit
exception_code = 0x02
```

## Register decoder

`ModbusRegisterDecoder` decodes numeric values from raw 16-bit Modbus registers.

Header:

```text id="qc457u"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_register_decoder.hpp
```

Implementation:

```text id="z7nime"
backend/libs/dispatcher-modbus/src/modbus_register_decoder.cpp
```

Supported value types:

```text id="agjm1j"
uint16
int16
uint32
int32
float32
```

Supported word orders:

```text id="h8bldu"
high_word_first
low_word_first
```

Supported operations:

```text id="lhl8yr"
decode_uint16(...)
decode_int16(...)
decode_uint32(...)
decode_int32(...)
decode_float32(...)
decode_numeric(...)
```

Validation behavior:

```text id="snv2xj"
empty register buffers are rejected
offset outside buffer is rejected
too-small buffer for 32-bit values is rejected
```

Example:

```cpp id="jbgxim"
const std::vector<std::uint16_t> registers{
    0x4148,
    0x0000
};

const auto value =
    dispatcher::modbus::ModbusRegisterDecoder::decode_float32(
        registers,
        0,
        dispatcher::modbus::ModbusWordOrder::high_word_first
    );
```

Expected decoded value:

```text id="f1el90"
12.5
```

## Transport abstraction

Transport boundary is defined by:

```text id="l7nify"
IModbusTcpTransport
```

Header:

```text id="p82d1k"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_tcp_transport.hpp
```

The interface exposes one operation:

```cpp id="ybmr23"
[[nodiscard]] virtual ModbusBytes exchange(
    const ModbusBytes& request_frame
) = 0;
```

This boundary allows the protocol/client/adapter logic to be tested without a real TCP socket.

`ModbusTcpEndpoint` currently stores:

```text id="r6u1ke"
host
port
```

Default endpoint:

```text id="umujm4"
127.0.0.1:502
```

## ModbusTcpClient

`ModbusTcpClient` builds request frames, calls the transport, decodes response frames, and validates response correlation.

Header:

```text id="tmokb9"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_tcp_client.hpp
```

Implementation:

```text id="vmktwj"
backend/libs/dispatcher-modbus/src/modbus_tcp_client.cpp
```

Supported operations:

```text id="dg7qxr"
read_holding_registers(...)
read_input_registers(...)
next_transaction_id()
```

The client validates:

```text id="oljs88"
transaction_id
unit_id
function_code
expected register count
exception response transaction_id
exception response unit_id
```

Invalid request parameters are rejected before the transport is called.

## Polling configuration

Polling configuration is defined in:

```text id="yiwccv"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_polling_config.hpp
```

Implementation:

```text id="kb2ipr"
backend/libs/dispatcher-modbus/src/modbus_polling_config.cpp
```

Main types:

```text id="m4on2o"
ModbusTagMapping
ModbusPollingConfiguration
ModbusPollRequest
ModbusPollingPlanBuilder
```

### ModbusTagMapping

A mapping connects a Dispatcher tag id to a Modbus register.

Fields:

```text id="xd0do8"
tag_id
unit_id
function_code
address
value_type
word_order
scale
offset
enabled
```

### ModbusPollingConfiguration

Configuration fields:

```text id="f2lfyk"
endpoint
poll_interval_ms
timeout_ms
max_registers_per_request
mappings
```

### Read plan generation

`ModbusPollingPlanBuilder::build_read_plan(...)` creates a list of `ModbusPollRequest`.

Mappings can be merged into the same request when they share:

```text id="bidnrk"
unit_id
function_code
compatible address range
max_registers_per_request limit
```

The builder:

```text id="et0q49"
skips disabled mappings
sorts enabled mappings
merges adjacent mappings
merges overlapping mappings
does not merge different unit ids
does not merge different function codes
does not exceed max_registers_per_request
```

Validation rules:

```text id="ijxbix"
endpoint host must not be empty
endpoint port must not be zero
poll_interval_ms must not be zero
timeout_ms must not be zero
max_registers_per_request must be between 1 and 125
enabled tag_id must not be empty
unit_id must not be zero
function_code must be read_holding_registers or read_input_registers
register range must not overflow the Modbus address space
enabled tag ids must be unique
```

## Telemetry adapter

`ModbusTelemetryAdapter` performs one polling cycle.

Header:

```text id="pvwucv"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_telemetry_adapter.hpp
```

Implementation:

```text id="mw457n"
backend/libs/dispatcher-modbus/src/modbus_telemetry_adapter.cpp
```

Main result types:

```text id="w40fan"
ModbusTelemetrySample
ModbusTelemetryPollError
ModbusTelemetryPollResult
```

Supported operations:

```text id="jl9ks8"
configuration()
read_plan()
poll_once()
```

The adapter:

```text id="hhc11a"
builds read_plan during construction
executes each ModbusPollRequest
reads holding registers
reads input registers
decodes register values
applies scale and offset
returns good-quality samples
records request-level errors
continues polling after one request fails
```

Sample fields:

```text id="oy8xnn"
tag_id
value
quality
source
unit_id
function_code
address
value_type
```

Default sample source:

```text id="yia53j"
modbus-tcp
```

Default successful quality:

```text id="meqggb"
good
```

Poll result behavior:

```text id="zlhtxj"
success() returns true when errors is empty
has_samples() returns true when samples is not empty
```

## Simulated TCP transport

`ModbusSimulatedTcpTransport` is an in-memory implementation of `IModbusTcpTransport`.

Header:

```text id="t62vma"
backend/libs/dispatcher-modbus/include/dispatcher/modbus/modbus_simulated_tcp_transport.hpp
```

Implementation:

```text id="h8yw9f"
backend/libs/dispatcher-modbus/src/modbus_simulated_tcp_transport.cpp
```

Supported operations:

```text id="w908bu"
exchange(...)
set_holding_register(...)
set_input_register(...)
set_holding_registers(...)
set_input_registers(...)
clear()
exchange_count()
requests()
```

The simulator supports:

```text id="p4cifm"
Read Holding Registers
Read Input Registers
missing register exception response
unsupported function exception response
request capture
exchange count
```

The simulator rejects malformed request frames and invalid protocol ids.

The simulator allows smoke tests for:

```text id="x832au"
ModbusTelemetryAdapter
ModbusTcpClient
ModbusFrameCodec
ModbusRegisterDecoder
```

without starting a real network server.

## Example: adapter with simulator

```cpp id="jgmuwa"
dispatcher::modbus::ModbusSimulatedTcpTransport transport;

transport.set_holding_registers(
    1,
    10,
    {
        100,
        250
    }
);

dispatcher::modbus::ModbusPollingConfiguration configuration;

configuration.endpoint.host = "127.0.0.1";
configuration.endpoint.port = 502;
configuration.poll_interval_ms = 1000;
configuration.timeout_ms = 500;
configuration.max_registers_per_request = 125;

dispatcher::modbus::ModbusTagMapping pressure;

pressure.tag_id = "pump.pressure";
pressure.unit_id = 1;
pressure.function_code =
    dispatcher::modbus::ModbusFunctionCode::read_holding_registers;
pressure.address = 10;
pressure.value_type =
    dispatcher::modbus::ModbusRegisterValueType::uint16;

dispatcher::modbus::ModbusTagMapping temperature;

temperature.tag_id = "pump.temperature";
temperature.unit_id = 1;
temperature.function_code =
    dispatcher::modbus::ModbusFunctionCode::read_holding_registers;
temperature.address = 11;
temperature.value_type =
    dispatcher::modbus::ModbusRegisterValueType::uint16;

configuration.mappings.push_back(
    pressure
);

configuration.mappings.push_back(
    temperature
);

dispatcher::modbus::ModbusTelemetryAdapter adapter{
    configuration,
    transport
};

const auto result =
    adapter.poll_once();
```

Expected result:

```text id="mxw2rl"
result.success() == true
result.samples.size() == 2
pump.pressure == 100
pump.temperature == 250
```

## Test coverage

Modbus tests are part of:

```text id="xoo5vy"
dispatcher-modbus-tests
```

Current test files:

```text id="iapbr4"
tests/unit/modbus_frame_codec_tests.cpp
tests/unit/modbus_register_decoder_tests.cpp
tests/unit/modbus_tcp_client_tests.cpp
tests/unit/modbus_polling_config_tests.cpp
tests/unit/modbus_telemetry_adapter_tests.cpp
tests/unit/modbus_simulated_tcp_transport_tests.cpp
```

Coverage includes:

```text id="ty1sfi"
Read Holding Registers request encoding
Read Input Registers request encoding
Read Registers response decoding
Modbus exception response detection
Modbus exception response decoding
invalid frame rejection
uint16 decoding
int16 decoding
uint32 decoding
int32 decoding
float32 decoding
word order handling
transport abstraction behavior
transaction id validation
unit id validation
function code validation
register count validation
polling configuration validation
read plan merge behavior
scale and offset handling
request-level polling errors
continue after request error
simulated holding register reads
simulated input register reads
simulated exception responses
adapter smoke test through simulator
```

## Verification commands

Configure:

```powershell id="z0qzuk"
cmake --preset windows-vs-debug
```

Build:

```powershell id="lg65kr"
cmake --build --preset windows-vs-debug
```

Run backend tests:

```powershell id="vf9ye8"
ctest --preset windows-vs-debug
```

Expected result:

```text id="utct8s"
100% tests passed, 0 tests failed out of 14
```

Frontend build check:

```powershell id="rcw9h7"
dotnet build frontend\Dispatcher.Frontend.slnx
```

E2E smoke:

```powershell id="yjj62s"
.\out\build\windows-vs-debug\backend\apps\dispatcher-e2e-smoke\Debug\dispatcher-e2e-smoke.exe 10000
```

## Known limitations

Modbus TCP adapter v1 is a protocol and adapter foundation.

Accepted limitations:

```text id="wvk72k"
No real TCP socket transport exists yet.
No continuous polling loop exists yet.
No DispatcherRuntime integration exists yet.
No telemetry ingest integration exists yet.
No SQLite history persistence integration exists yet.
No frontend configuration screen exists yet.
No HTTP API endpoints expose Modbus adapter state.
No JSON/YAML Modbus configuration import exists yet.
No reconnect/backoff policy exists yet.
No timeout implementation exists in the in-memory transport boundary.
No per-device health state exists yet.
No metrics are emitted yet.
No Modbus write functions exist.
No coil reads exist.
No discrete input reads exist.
No string decoding exists.
No bit field decoding exists.
No byte order variants beyond word order exist yet.
No production TCP security model exists.
```

These limitations are accepted for Modbus TCP adapter v1 and should be addressed in later runtime, deployment, and UI stages.

## Future integration targets

Likely future work:

```text id="cpb897"
add real Boost.Asio TCP transport
add continuous polling scheduler
connect ModbusTelemetryAdapter to telemetry ingest
persist Modbus samples through SqliteHistoryStorage
add runtime health reporting for Modbus adapters
add HTTP API endpoints for adapter status
add configuration import/export for Modbus mappings
add frontend diagnostics page for telemetry adapters
add operational docs for timeout, reconnect, polling intervals and register maps
```
