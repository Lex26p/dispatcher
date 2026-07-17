# Auth/audit integration foundation v1

## Summary

Auth/audit integration foundation v1 introduces a dedicated backend library for audit event modeling, audit recording, authorization policy evaluation, authorization decision auditing, and adapter foundations for HTTP/API and runtime-facing operations.

The library is implemented as:

```text id="nc12td"
backend/libs/dispatcher-auth-audit
```

This version does not yet enforce authorization in the live HTTP server or `DispatcherRuntime`. It provides tested foundations that later stages can wire into runtime actions, HTTP endpoints, persistent audit storage, deployment configuration, and frontend diagnostics.

## Library

Main library:

```text id="rfey76"
dispatcher-auth-audit
```

Location:

```text id="a87u0r"
backend/libs/dispatcher-auth-audit
```

Main umbrella header:

```cpp id="p40brn"
#include <dispatcher/auth/audit/auth_audit.hpp>
```

The library currently includes:

```text id="rikujw"
AuthAuditError
AuthAuditActorType
AuthAuditAction
AuthAuditOutcome
AuthAuditSeverity
AuthAuditRecordStatus
AuthAuditActor
AuthAuditResource
AuthAuditEvent
AuthAuditRecordResult
IAuthAuditSink
AuthAuditValidator
InMemoryAuthAuditSink
AuthAuditLogger
AuthorizationPermission
AuthorizationDecisionEffect
AuthorizationSubject
AuthorizationRequest
AuthorizationDecision
AuthorizationPolicyRule
AuthorizationPolicy
AuthorizationPolicyEvaluator
AuthenticatedOperationContext
AuthenticatedOperationContextBuilder
AuthorizationAuditOptions
AuthorizationAuditRecord
AuthorizationAuditRecorder
HttpAuthAuditRequestContext
HttpAuthAuditEndpointRule
HttpAuthAuditMappingResult
HttpAuthAuditAuthorizeResult
HttpAuthAuditAdapter
RuntimeAuthAuditOperation
RuntimeAuthAuditRequestContext
RuntimeAuthAuditOperationRule
RuntimeAuthAuditMappingResult
RuntimeAuthAuditAuthorizeResult
RuntimeAuthAuditAdapter
```

## Scope

This version supports:

```text id="vs47w2"
audit event model
audit result model
audit event validation
audit result validation
audit sink interface
in-memory audit sink
audit logger
batch audit recording
authorization permission model
authorization request model
authorization decision model
authorization policy rules
authorization policy evaluator
deny-overrides-allow behavior
direct subject permissions
administrator permission
authenticated operation context
authorization decision to audit event mapping
evaluate-and-record authorization flow
HTTP/API request context mapping foundation
runtime request context mapping foundation
HTTP/API authorization and audit smoke flow
runtime authorization and audit smoke flow
```

This version intentionally does not yet support:

```text id="go6mlf"
live HTTP endpoint enforcement
live DispatcherRuntime enforcement
persistent audit storage
SQLite audit event repository
auth token parsing
session management
passwords
JWT validation
RBAC configuration import/export
frontend auth screens
frontend audit viewer
operator login/logout flow
audit retention policy
audit signing
audit tamper evidence
```

## Error model

Auth/audit-specific errors are represented by:

```text id="f5kc27"
dispatcher::auth::audit::AuthAuditError
```

Header:

```text id="u658dl"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_audit_error.hpp
```

`AuthAuditError` derives from:

```text id="liqies"
std::runtime_error
```

This exception is used for invalid audit events, invalid audit results, invalid policies, invalid authorization requests, invalid operation contexts, invalid HTTP/runtime adapter rules, and invalid mapping inputs.

## Audit types

Core audit types are defined in:

```text id="b0yeux"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_audit_types.hpp
```

Main enums:

```text id="urpfqr"
AuthAuditActorType
AuthAuditAction
AuthAuditOutcome
AuthAuditSeverity
AuthAuditRecordStatus
```

Supported actor types:

```text id="tw47ca"
system
operator_user
service
anonymous
```

Supported audit actions:

```text id="u5skm4"
unknown
runtime_read
runtime_control
alarm_acknowledge
alarm_shelve
alarm_unshelve
configuration_import
configuration_export
notification_send
authorization_check
login
logout
```

`unknown` exists as a default enum value, but valid audit events must not use it as the final action.

Supported outcomes:

```text id="vdv0va"
success
denied
failed
```

Supported severities:

```text id="olvk0d"
info
warning
critical
```

Supported record statuses:

```text id="fdlfv7"
accepted
failed
skipped
```

## AuthAuditEvent

`AuthAuditEvent` is the normalized audit event DTO.

Fields:

```text id="l0lkf2"
event_id
correlation_id
source
actor
action
outcome
severity
resource
reason
diagnostic_message
attributes
occurred_at
```

`correlation_id` links the audit event to an upstream operation, HTTP request, runtime operation, alarm action, configuration action, or authorization request.

`attributes` stores additional provider-neutral context such as:

```text id="n9l9ki"
http.method
http.path
runtime.operation
required_permission
decision_effect
decision_reason
client_address
user_agent
operation_id
authorization_request_id
```

## AuthAuditRecordResult

`AuthAuditRecordResult` is the normalized result returned by audit sinks.

Fields:

```text id="d5whth"
status
provider_record_id
error_message
diagnostic_message
completed_at
```

Factory helpers:

```text id="baqbbf"
AuthAuditRecordResult::accepted(...)
AuthAuditRecordResult::failed(...)
AuthAuditRecordResult::skipped(...)
```

Convenience methods:

```text id="psbupc"
success()
failure()
```

Validation rules:

```text id="kbn4kf"
failed result must contain error_message
accepted result must not contain error_message
skipped result must contain diagnostic_message
status must be valid
```

## Audit sink interface

Audit sinks implement:

```text id="va27ve"
IAuthAuditSink
```

Header:

```text id="j6hfba"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_audit_sink.hpp
```

Interface:

```cpp id="csbwvn"
class IAuthAuditSink
{
public:
    virtual ~IAuthAuditSink() = default;

    [[nodiscard]] virtual std::string sink_name() const = 0;

    [[nodiscard]] virtual AuthAuditRecordResult record(
        const AuthAuditEvent& event
    ) = 0;
};
```

## Validator

`AuthAuditValidator` validates audit events, audit record results, and enum values.

Header:

```text id="zkvnh4"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_audit_validator.hpp
```

Implementation:

```text id="cxm003"
backend/libs/dispatcher-auth-audit/src/auth_audit_validator.cpp
```

Supported operations:

```text id="gsaorz"
validate_event(...)
validate_record_result(...)
actor_type_to_string(...)
action_to_string(...)
outcome_to_string(...)
severity_to_string(...)
record_status_to_string(...)
```

Audit event validation checks:

```text id="yewluo"
event_id must not be empty
source must not be empty
actor_type must be valid
actor_id must not be empty
action must be valid
action must not be unknown
outcome must be valid
severity must be valid
resource_type must not be empty
resource_id must not be empty
non-success outcome must contain reason
```

## In-memory audit sink

`InMemoryAuthAuditSink` is a test/local diagnostics sink.

Header:

```text id="wsnq3m"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/in_memory_auth_audit_sink.hpp
```

Implementation:

```text id="ke0lxs"
backend/libs/dispatcher-auth-audit/src/in_memory_auth_audit_sink.cpp
```

Supported operations:

```text id="s6zdot"
record(...)
set_failure(...)
clear_failure()
set_recording_enabled(...)
clear()
recording_enabled()
record_attempt_count()
recorded_events()
```

Behavior:

```text id="han33y"
validates events
records accepted events in memory
tracks record attempts
can be forced to fail
can skip recording when disabled
can clear state
returns provider_record_id as sink_name:event_id
```

## Audit logger

`AuthAuditLogger` is the recording facade used by higher-level components.

Header:

```text id="jetr1g"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_audit_logger.hpp
```

Implementation:

```text id="xhm36k"
backend/libs/dispatcher-auth-audit/src/auth_audit_logger.cpp
```

Supported operations:

```text id="ns8r3q"
set_enabled(...)
enabled()
sink_name()
record(...)
record_batch(...)
```

Logger behavior:

```text id="tq0rtu"
rejects empty sink_name on construction
validates event before recording
can be globally disabled
returns skipped result when disabled
records through configured sink
validates sink result
converts sink exceptions to failed results
converts invalid sink results to failed results
preserves batch order
throws on invalid batch event after prior valid records are already written
```

The logger does not own sink lifetime. The sink must outlive logger usage.

## Authorization model

Authorization types are defined in:

```text id="tlln76"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_authorization_policy.hpp
```

Implementation:

```text id="i4dvra"
backend/libs/dispatcher-auth-audit/src/auth_authorization_policy.cpp
```

Main authorization types:

```text id="dyxehc"
AuthorizationPermission
AuthorizationDecisionEffect
AuthorizationSubject
AuthorizationRequest
AuthorizationDecision
AuthorizationPolicyRule
AuthorizationPolicy
AuthorizationPolicyEvaluator
```

Supported permissions:

```text id="qk1xqj"
runtime_read
runtime_control
alarm_acknowledge
alarm_shelve
alarm_unshelve
configuration_import
configuration_export
notification_send
audit_read
audit_write
administrator
```

Supported decision effects:

```text id="nu36c8"
allow
deny
```

## AuthorizationRequest

`AuthorizationRequest` represents one authorization check.

Fields:

```text id="r30zqu"
request_id
correlation_id
source
subject
action
resource
required_permission
attributes
```

Validation checks:

```text id="e6z8qe"
request_id must not be empty
source must not be empty
subject_id must not be empty
subject_type must be valid
action must not be unknown
action must be valid
resource_type must not be empty
resource_id must not be empty
required_permission must be valid
```

## AuthorizationPolicy

`AuthorizationPolicy` controls authorization decisions.

Fields:

```text id="yeo5ee"
default_effect
allow_direct_subject_permissions
rules
```

Default policy:

```text id="n9d18o"
default_effect = deny
allow_direct_subject_permissions = true
rules = empty
```

This means a subject can be allowed by direct permissions, otherwise the result is denied by default.

## AuthorizationPolicyRule

`AuthorizationPolicyRule` fields:

```text id="f4clw9"
rule_id
effect
permission
role
resource_type
resource_id
enabled
```

Rule behavior:

```text id="yzbkem"
disabled rules are ignored
empty role matches any subject role
non-empty role must match subject role
empty resource_type matches any resource type
empty resource_id matches any resource id
administrator permission matches any requested permission
deny rules are evaluated before allow rules
```

Deny-overrides-allow behavior is intentional.

## AuthorizationDecision

`AuthorizationDecision` contains the result of a policy evaluation.

Fields:

```text id="fzxfby"
request_id
correlation_id
subject_id
action
resource
required_permission
effect
reason
diagnostic_message
decided_at
```

Factory helpers:

```text id="x5ph8n"
AuthorizationDecision::allowed(...)
AuthorizationDecision::denied(...)
```

Convenience methods:

```text id="svds3w"
allowed()
denied()
```

Common decision reasons:

```text id="zzt6x0"
allowed_by_policy_rule
denied_by_policy_rule
allowed_by_subject_permission
allowed_by_default_policy
denied_by_default_policy
```

## Authenticated operation context

Operation context types are defined in:

```text id="bwp2st"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/auth_operation_context.hpp
```

Implementation:

```text id="zq5lkl"
backend/libs/dispatcher-auth-audit/src/auth_operation_context.cpp
```

Main types:

```text id="kkdtjm"
AuthenticatedOperationContext
AuthenticatedOperationContextBuilder
```

Context fields:

```text id="z5ct74"
operation_id
correlation_id
source
actor
client_address
user_agent
attributes
```

Builder operations:

```text id="sc9b8e"
system_context(...)
operator_context(...)
validate_context(...)
to_authorization_subject(...)
```

Validation checks:

```text id="zm8lb5"
operation_id must not be empty
source must not be empty
actor_id must not be empty
actor_type must be valid
```

The context builder is used by HTTP/API and runtime adapter foundations to normalize actor and operation metadata before building an `AuthorizationRequest`.

## Authorization decision auditing

Authorization decision audit mapping is defined in:

```text id="e86w6c"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/authorization_audit_recorder.hpp
```

Implementation:

```text id="l45hm1"
backend/libs/dispatcher-auth-audit/src/authorization_audit_recorder.cpp
```

Main types:

```text id="uly1hc"
AuthorizationAuditOptions
AuthorizationAuditRecord
AuthorizationAuditRecorder
```

Options:

```text id="fliqd1"
record_allowed
record_denied
```

Default options:

```text id="jpk8oi"
record_allowed = true
record_denied = true
```

Supported operations:

```text id="ris1ca"
validate_options(...)
validate_decision(...)
build_event(...)
record_decision(...)
evaluate_and_record(...)
```

Decision to audit mapping:

```text id="u0m2be"
allowed decision -> AuthAuditOutcome::success
denied decision -> AuthAuditOutcome::denied
allowed decision -> AuthAuditSeverity::info
denied decision -> AuthAuditSeverity::warning
audit action -> authorization_check
```

Generated audit event id format:

```text id="pphqwf"
authorization:<authorization_request_id>
```

Automatically added attributes:

```text id="t282qm"
operation_id
authorization_request_id
authorization_subject_id
authorization_action
required_permission
decision_effect
decision_reason
client_address
user_agent
context.<key>
```

`evaluate_and_record(...)` first evaluates authorization and then records the decision through `AuthAuditLogger`.

## HTTP/API auth-audit adapter foundation

HTTP/API adapter foundation is defined in:

```text id="f3rq03"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/http_auth_audit_adapter.hpp
```

Implementation:

```text id="fms4i7"
backend/libs/dispatcher-auth-audit/src/http_auth_audit_adapter.cpp
```

Main types:

```text id="aiejz7"
HttpAuthAuditRequestContext
HttpAuthAuditEndpointRule
HttpAuthAuditMappingResult
HttpAuthAuditAuthorizeResult
HttpAuthAuditAdapter
```

`HttpAuthAuditRequestContext` fields:

```text id="ce31qp"
request_id
correlation_id
source
method
path
actor_id
actor_display_name
actor_type
roles
permissions
client_address
user_agent
attributes
```

`HttpAuthAuditEndpointRule` fields:

```text id="h8xryc"
rule_id
method
path_prefix
action
resource
required_permission
enabled
require_authenticated
```

Adapter operations:

```text id="g3u3j3"
map_request(...)
authorize_and_audit(...)
validate_request_context(...)
validate_endpoint_rule(...)
```

Matching behavior:

```text id="kscf34"
method must match exactly
path must start with path_prefix
disabled endpoint rules are ignored
longest matching path_prefix wins
authenticated endpoint rejects anonymous actor
public endpoint can map anonymous actor
```

Mapping result:

```text id="b8yfs9"
HTTP request context -> AuthenticatedOperationContext
HTTP request context + endpoint rule -> AuthorizationRequest
AuthorizationRequest + evaluator + logger -> AuthorizationAuditRecord
```

This adapter is a foundation only. It is not yet called by the live Boost.Beast HTTP server.

## Runtime auth-audit adapter foundation

Runtime adapter foundation is defined in:

```text id="l7mhv4"
backend/libs/dispatcher-auth-audit/include/dispatcher/auth/audit/runtime_auth_audit_adapter.hpp
```

Implementation:

```text id="f3g1e5"
backend/libs/dispatcher-auth-audit/src/runtime_auth_audit_adapter.cpp
```

Main types:

```text id="nmd1lc"
RuntimeAuthAuditOperation
RuntimeAuthAuditRequestContext
RuntimeAuthAuditOperationRule
RuntimeAuthAuditMappingResult
RuntimeAuthAuditAuthorizeResult
RuntimeAuthAuditAdapter
```

Supported runtime operation names:

```text id="drctyd"
runtime_read
runtime_control
alarm_acknowledge
alarm_shelve
alarm_unshelve
configuration_import
configuration_export
notification_send
```

`RuntimeAuthAuditRequestContext` fields:

```text id="rq27jc"
operation_id
correlation_id
source
operation
actor_id
actor_display_name
actor_type
roles
permissions
resource
client_address
user_agent
attributes
```

`RuntimeAuthAuditOperationRule` fields:

```text id="bemk2g"
rule_id
operation
action
resource
required_permission
enabled
require_authenticated
```

Adapter operations:

```text id="v7od8i"
map_request(...)
authorize_and_audit(...)
validate_request_context(...)
validate_operation_rule(...)
operation_to_string(...)
```

Runtime mapping behavior:

```text id="bwuzj2"
operation must match operation rule
disabled operation rules are ignored
authenticated operation rejects anonymous actor
public operation can map anonymous actor
request resource can override rule resource
runtime operation is added to attributes
operation rule id is added to attributes
```

This adapter is a foundation only. It is not yet called by live `DispatcherRuntime`.

## Test coverage

Auth/audit tests are part of:

```text id="tpe7e4"
dispatcher-auth-audit-tests
```

Current test files:

```text id="uymtfc"
tests/unit/auth_audit_foundation_tests.cpp
tests/unit/auth_audit_logger_tests.cpp
tests/unit/auth_authorization_policy_tests.cpp
tests/unit/auth_authorization_audit_tests.cpp
tests/unit/auth_http_adapter_tests.cpp
tests/unit/auth_runtime_integration_tests.cpp
```

Coverage includes:

```text id="pocwsw"
audit event validation
audit record result validation
audit enum string conversion
audit sink interface behavior
in-memory audit sink recording
in-memory audit sink forced failure
in-memory audit sink disabled mode
audit logger recording
audit logger disabled mode
audit logger sink exception handling
audit logger invalid result handling
batch audit recording
authorization request validation
authorization rule validation
authorization policy validation
direct subject permission allow
missing permission deny
allow rule behavior
deny rule overrides allow rule
disabled rule ignored
role matching
resource matching
administrator permission
administrator rule permission
default allow policy
disabled direct subject permissions
operation context validation
context to authorization subject conversion
authorization decision validation
authorization decision to audit event mapping
record allowed decision
record denied decision
skip allowed audit
skip denied audit
evaluate and record allowed decision
evaluate and record denied decision
audit sink failure propagation
HTTP request context validation
HTTP endpoint rule validation
HTTP mapping
HTTP longest path prefix matching
HTTP authenticated/public endpoint behavior
HTTP authorize-and-audit allowed flow
HTTP authorize-and-audit denied flow
runtime request context validation
runtime operation rule validation
runtime mapping
runtime resource override
runtime authenticated/public operation behavior
runtime authorize-and-audit runtime_read flow
runtime authorize-and-audit runtime_control flow
runtime authorize-and-audit alarm_acknowledge flow
runtime authorize-and-audit notification_send flow
multiple runtime operations smoke flow
```

## Verification commands

Configure:

```powershell id="ct8nzc"
cmake --preset windows-vs-debug
```

Build:

```powershell id="e5d0eb"
cmake --build --preset windows-vs-debug
```

Run backend tests:

```powershell id="vka3f2"
ctest --preset windows-vs-debug
```

Expected result:

```text id="py2wqh"
100% tests passed, 0 tests failed out of 16
```

Frontend build check:

```powershell id="vep3h0"
dotnet build frontend\Dispatcher.Frontend.slnx
```

E2E smoke:

```powershell id="l0wbgr"
.\out\build\windows-vs-debug\backend\apps\dispatcher-e2e-smoke\Debug\dispatcher-e2e-smoke.exe 10000
```

## Known limitations

Auth/audit integration foundation v1 is not yet production enforcement.

Accepted limitations:

```text id="m3z31q"
No live HTTP endpoint authorization enforcement yet.
No live DispatcherRuntime authorization enforcement yet.
No persistent audit sink exists yet.
No SQLite audit repository exists yet.
No auth token parser exists yet.
No JWT validation exists yet.
No session model exists yet.
No password or login flow exists yet.
No frontend login screen exists yet.
No frontend audit event viewer exists yet.
No HTTP API exposes audit records yet.
No policy import/export exists yet.
No role management UI exists yet.
No audit retention policy exists yet.
No audit signing exists yet.
No tamper-evident audit chain exists yet.
No per-tenant authorization exists yet.
No external identity provider integration exists yet.
```

These limitations are accepted for auth/audit integration foundation v1.

## Future integration targets

Likely future work:

```text id="qnpb81"
connect HttpAuthAuditAdapter to Boost.Beast HTTP server
enforce authorization on HTTP endpoints
connect RuntimeAuthAuditAdapter to runtime operations
persist audit events to SQLite
add audit event query API
add audit diagnostics API
add policy configuration import/export
add JWT/session validation
add operator login/logout events
connect notification delivery audit
connect alarm acknowledgement audit
connect configuration import/export audit
add deployment configuration for auth/audit
add frontend audit diagnostics pages after UX planning
```
