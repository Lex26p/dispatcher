# Notification delivery backend v1

## Summary

Notification delivery backend v1 introduces the delivery-side foundation for Dispatcher notifications.

The delivery backend is implemented as a separate backend library:

```text
backend/libs/dispatcher-notification-delivery
```

This library provides a common notification delivery message model, delivery result model, channel interface, in-memory delivery channel, dispatcher, retry executor, file-based delivery channel, webhook delivery foundation, and an alarm-facing message builder.

This version does not yet connect directly to the live alarm engine or alarm routing runtime. It provides a tested delivery layer that later stages can wire into runtime alarm workflows, HTTP APIs, audit, and deployment configuration.

## Library

Main library:

```text
dispatcher-notification-delivery
```

Location:

```text
backend/libs/dispatcher-notification-delivery
```

Main umbrella header:

```cpp
#include <dispatcher/notification/delivery/notification_delivery.hpp>
```

The library currently includes:

```text
NotificationDeliveryError
NotificationDeliveryChannelType
NotificationDeliveryPriority
NotificationDeliveryStatus
NotificationDeliveryRecipient
NotificationDeliveryMessage
NotificationDeliveryResult
INotificationDeliveryChannel
NotificationDeliveryValidator
InMemoryNotificationDeliveryChannel
NotificationDeliveryDispatcher
NotificationDeliveryRetryPolicy
NotificationDeliveryAttempt
NotificationDeliveryExecutionResult
NotificationDeliveryRetryExecutor
FileNotificationDeliveryOptions
FileNotificationDeliveryChannel
WebhookHttpRequest
WebhookHttpResponse
IWebhookHttpClient
WebhookNotificationDeliveryOptions
WebhookNotificationDeliveryChannel
AlarmNotificationSeverity
AlarmNotificationDeliveryRequest
AlarmNotificationDeliveryMessageBuilder
```

## Scope

This version supports:

```text
common delivery message model
common delivery result model
delivery message validation
delivery result validation
delivery channel interface
in-memory delivery channel
delivery channel registration
delivery dispatch by channel type
batch delivery
missing channel handling
channel exception handling
retry policy
attempt tracking
batch retry delivery
file-based notification delivery
webhook request construction through an abstract HTTP client
alarm notification request to delivery message conversion
alarm delivery smoke tests through in-memory, file, and webhook channels
```

This version intentionally does not yet support:

```text
SMTP delivery
SMS delivery
real webhook HTTP client implementation
live alarm engine integration
live notification routing integration
scheduler-based retries
async delivery workers
durable delivery queue
delivery persistence
delivery audit integration
frontend delivery configuration
HTTP API for delivery state
```

## Error model

Delivery-specific errors are represented by:

```text
dispatcher::notification::delivery::NotificationDeliveryError
```

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/notification_delivery_error.hpp
```

`NotificationDeliveryError` derives from:

```text
std::runtime_error
```

This exception is used for invalid local configuration, invalid messages, invalid results, and invalid retry policy.

## Delivery types

Core delivery types are defined in:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/notification_delivery_types.hpp
```

Main enums:

```text
NotificationDeliveryChannelType
NotificationDeliveryPriority
NotificationDeliveryStatus
```

Supported channel types:

```text
console
email
sms
webhook
file
test
```

The presence of a channel type does not mean that a concrete production channel exists yet. In v1, concrete implemented channels are:

```text
test/in-memory
file
webhook foundation through IWebhookHttpClient
```

Supported priorities:

```text
low
normal
high
critical
```

Supported delivery statuses:

```text
pending
delivered
failed
skipped
```

## NotificationDeliveryMessage

`NotificationDeliveryMessage` is the normalized message DTO that all delivery channels receive.

Fields:

```text
message_id
correlation_id
source
channel_type
priority
recipient
subject
body
attributes
created_at
```

`correlation_id` is intended to link the message to an upstream domain object, such as an alarm id.

`attributes` stores provider-neutral metadata, such as:

```text
alarm_id
tag_id
device_id
site
severity
```

## NotificationDeliveryResult

`NotificationDeliveryResult` is the normalized result returned by delivery channels.

Fields:

```text
status
provider_message_id
error_message
diagnostic_message
completed_at
```

Factory helpers:

```text
NotificationDeliveryResult::delivered(...)
NotificationDeliveryResult::failed(...)
NotificationDeliveryResult::skipped(...)
```

Convenience methods:

```text
success()
failure()
```

Validation rules:

```text
failed result must contain error_message
delivered result must not contain error_message
skipped result must contain diagnostic_message
status must be valid
```

## Delivery channel interface

Delivery channels implement:

```text
INotificationDeliveryChannel
```

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/notification_delivery_channel.hpp
```

Interface:

```cpp
class INotificationDeliveryChannel
{
public:
    virtual ~INotificationDeliveryChannel() = default;

    [[nodiscard]] virtual NotificationDeliveryChannelType channel_type() const noexcept = 0;

    [[nodiscard]] virtual std::string channel_name() const = 0;

    [[nodiscard]] virtual NotificationDeliveryResult deliver(
        const NotificationDeliveryMessage& message
    ) = 0;
};
```

## Validator

`NotificationDeliveryValidator` validates messages, results, and enum values.

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/notification_delivery_validator.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/notification_delivery_validator.cpp
```

Supported operations:

```text
validate_message(...)
validate_result(...)
channel_type_to_string(...)
priority_to_string(...)
status_to_string(...)
```

Message validation checks:

```text
message_id must not be empty
source must not be empty
channel_type must be valid
priority must be valid
recipient address must not be empty
subject must not be empty
body must not be empty
```

## In-memory channel

`InMemoryNotificationDeliveryChannel` is a safe test channel.

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/in_memory_notification_delivery_channel.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/in_memory_notification_delivery_channel.cpp
```

Supported operations:

```text
deliver(...)
set_failure(...)
clear_failure()
clear()
delivered_messages()
delivery_attempt_count()
```

Behavior:

```text
validates message
skips wrong channel_type
can be forced to fail
records delivered messages
counts delivery attempts
returns provider_message_id as channel_name:message_id
```

This channel is intended for unit tests, smoke tests, and future local diagnostics.

## Dispatcher

`NotificationDeliveryDispatcher` routes messages to registered delivery channels.

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/notification_delivery_dispatcher.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/notification_delivery_dispatcher.cpp
```

Supported operations:

```text
register_channel(...)
clear_channels()
has_channel(...)
channel_count()
deliver(...)
deliver_batch(...)
```

Dispatcher behavior:

```text
rejects empty channel_name
rejects duplicate channel type
validates message before delivery
returns failed result when channel is missing
calls matching channel by channel_type
validates channel result
converts channel exception to failed result
converts invalid channel result to failed result
preserves batch order
```

The dispatcher does not own channel lifetime. Registered channels must outlive the dispatcher use.

## Retry executor

`NotificationDeliveryRetryExecutor` retries delivery through a dispatcher according to a retry policy.

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/notification_delivery_retry.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/notification_delivery_retry.cpp
```

Main types:

```text
NotificationDeliveryRetryPolicy
NotificationDeliveryAttempt
NotificationDeliveryExecutionResult
NotificationDeliveryRetryExecutor
```

Retry policy fields:

```text
max_attempts
retry_failed
retry_skipped
```

Default policy:

```text
max_attempts = 3
retry_failed = true
retry_skipped = false
```

Policy validation:

```text
max_attempts must be greater than zero
max_attempts must not exceed 10
```

Executor operations:

```text
deliver_with_retry(...)
deliver_batch_with_retry(...)
```

Retry behavior:

```text
delivered result stops retry
failed result retries when retry_failed is true
skipped result retries only when retry_skipped is true
retry stops at max_attempts
invalid message is rejected before attempts
attempt history records every attempt
```

Attempt fields:

```text
attempt_number
status
provider_message_id
error_message
diagnostic_message
started_at
completed_at
```

This version does not sleep between attempts and does not implement backoff timing. It only records deterministic retry attempts.

## File delivery channel

`FileNotificationDeliveryChannel` writes notifications to a log file.

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/file_notification_delivery_channel.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/file_notification_delivery_channel.cpp
```

Options:

```text
directory
file_name
create_directories
append
```

Default options:

```text
directory = notifications
file_name = notifications.log
create_directories = true
append = true
```

Behavior:

```text
validates options on construction
validates message before delivery
skips wrong channel_type
creates output directory when enabled
writes notification entry to file
supports append mode
supports overwrite mode
sanitizes multiline field values
returns failed result on directory creation or file open/write failure
```

The file output format is line-oriented:

```text
--- notification ---
message_id=...
correlation_id=...
source=...
channel_type=file
priority=...
recipient_id=...
recipient_display_name=...
recipient_address=...
subject=...
body=...
attribute.<key>=<value>
--- end notification ---
```

This channel is useful for local development, smoke tests, and deployment environments where file-based integration is acceptable.

## Webhook delivery foundation

`WebhookNotificationDeliveryChannel` builds webhook HTTP requests and sends them through an abstract HTTP client.

Header:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/webhook_notification_delivery_channel.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/webhook_notification_delivery_channel.cpp
```

Main types:

```text
WebhookHttpRequest
WebhookHttpResponse
IWebhookHttpClient
WebhookNotificationDeliveryOptions
WebhookNotificationDeliveryChannel
```

Options:

```text
url
default_headers
timeout_ms
success_status_min
success_status_max
include_attributes
```

Behavior:

```text
validates options on construction
requires http:// or https:// URL
validates message before delivery
skips wrong channel_type
builds POST request
sets Content-Type application/json
includes default headers
builds JSON payload
escapes JSON string values
optionally excludes attributes
maps configured success status range to delivered result
maps non-success status to failed result
converts HTTP client exception to failed result
```

This version does not provide a real HTTP client implementation. Tests use a scripted `IWebhookHttpClient`.

## Alarm notification delivery bridge

Alarm-facing delivery bridge is defined in:

```text
backend/libs/dispatcher-notification-delivery/include/dispatcher/notification/delivery/alarm_notification_delivery.hpp
```

Implementation:

```text
backend/libs/dispatcher-notification-delivery/src/alarm_notification_delivery.cpp
```

Main types:

```text
AlarmNotificationSeverity
AlarmNotificationDeliveryRequest
AlarmNotificationDeliveryMessageBuilder
```

Supported severities:

```text
info
warning
major
critical
```

Severity to delivery priority mapping:

```text
info -> low
warning -> normal
major -> high
critical -> critical
```

`AlarmNotificationDeliveryRequest` fields:

```text
notification_id
alarm_id
tag_id
alarm_name
alarm_state
source
severity
channel_type
recipient
subject_prefix
body_details
attributes
```

Builder operations:

```text
build_message(...)
deliver(...)
deliver_with_retry(...)
priority_for_severity(...)
severity_to_string(...)
validate_request(...)
```

Message mapping:

```text
notification_id -> message_id
alarm_id -> correlation_id
source -> source
channel_type -> channel_type
severity -> priority
recipient -> recipient
generated subject -> subject
generated body -> body
alarm fields -> attributes
```

The builder automatically inserts or overwrites the following attributes:

```text
alarm_id
tag_id
alarm_name
alarm_state
severity
```

This bridge is intentionally small and does not directly subscribe to the live alarm engine. It is a tested adapter point for future runtime integration.

## Test coverage

Notification delivery tests are part of:

```text
dispatcher-notification-delivery-tests
```

Current test files:

```text
tests/unit/notification_delivery_foundation_tests.cpp
tests/unit/notification_delivery_dispatcher_tests.cpp
tests/unit/notification_delivery_retry_tests.cpp
tests/unit/notification_delivery_file_channel_tests.cpp
tests/unit/notification_delivery_webhook_channel_tests.cpp
tests/unit/notification_delivery_alarm_integration_tests.cpp
```

Coverage includes:

```text
message validation
result validation
enum string conversion
channel interface behavior
in-memory channel delivery
in-memory channel forced failure
dispatcher channel registration
duplicate channel rejection
missing channel handling
channel exception handling
invalid channel result handling
batch delivery
retry policy validation
retry attempts
retry failed results
optional retry skipped results
batch retry delivery
file channel directory creation
file channel append mode
file channel overwrite mode
file channel wrong-channel skip
file channel failure handling
file channel dispatcher integration
file channel retry integration
webhook option validation
webhook request construction
webhook JSON payload escaping
webhook attributes inclusion/exclusion
webhook success/failure mapping
webhook dispatcher integration
webhook retry integration
alarm request validation
alarm message building
alarm severity priority mapping
alarm in-memory smoke delivery
alarm file smoke delivery
alarm webhook smoke delivery
alarm retry smoke delivery
```

## Verification commands

Configure:

```powershell
cmake --preset windows-vs-debug
```

Build:

```powershell
cmake --build --preset windows-vs-debug
```

Run backend tests:

```powershell
ctest --preset windows-vs-debug
```

Expected result:

```text
100% tests passed, 0 tests failed out of 15
```

Frontend build check:

```powershell
dotnet build frontend\Dispatcher.Frontend.slnx
```

E2E smoke:

```powershell
.\out\build\windows-vs-debug\backend\apps\dispatcher-e2e-smoke\Debug\dispatcher-e2e-smoke.exe 10000
```

## Known limitations

Notification delivery backend v1 is a delivery foundation, not a production notification service.

Accepted limitations:

```text
No SMTP channel exists yet.
No SMS channel exists yet.
No real webhook HTTP client exists yet.
No live alarm engine integration exists yet.
No live alarm routing integration exists yet.
No durable delivery queue exists yet.
No delivery persistence exists yet.
No delivery audit integration exists yet.
No async worker exists yet.
No scheduler exists yet.
No timed backoff exists yet.
No per-recipient throttling exists yet.
No notification template engine exists yet.
No frontend delivery configuration screen exists yet.
No HTTP API exposes delivery state yet.
No secret management exists for webhook tokens.
No retry state survives process restart.
No provider-specific response parsing exists yet.
```

These limitations are accepted for notification delivery backend v1 and should be addressed in later integration, audit, deployment, and UI stages.

## Future integration targets

Likely future work:

```text
connect alarm routing decisions to NotificationDeliveryMessage
add durable delivery queue
persist delivery attempts
connect delivery events to audit log
add real Boost.Beast/Boost.Asio webhook HTTP client
add SMTP delivery channel
add delivery configuration import/export
add runtime delivery health reporting
add HTTP API endpoints for delivery diagnostics
add frontend diagnostics and configuration pages
add deployment configuration for file/webhook delivery
```
