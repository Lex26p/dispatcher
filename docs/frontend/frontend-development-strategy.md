# Frontend development strategy

## Purpose

This document defines the development strategy for the Dispatcher Blazor WebAssembly frontend.

The frontend is the operator console for the Dispatcher system. It must provide a clear, stable and operationally useful interface for monitoring runtime health, alarms, telemetry state and future operator actions.

The current frontend stage is a foundation stage. It provides the application shell, backend API configuration, typed API client, CORS-capable backend integration and basic operational pages.

## Current frontend architecture

The frontend is implemented as a standalone Blazor WebAssembly application:

```text id="y9az7s"
frontend/Dispatcher.Web
```

The frontend solution is:

```text id="j4xfye"
frontend/Dispatcher.Frontend.slnx
```

The frontend communicates with the C++ backend over HTTP.

Backend local development URL:

```text id="j6cxyr"
http://127.0.0.1:18080
```

Frontend local development URL:

```text id="n1yil7"
http://localhost:5077
```

The frontend does not host the C++ backend. The backend and frontend are separate processes.

## Architectural principles

### 1. Keep frontend and backend separated

The C++ backend owns runtime execution, alarm processing, telemetry ingestion, storage and future service packaging.

The Blazor frontend owns presentation, operator interaction, navigation and browser-side state.

The frontend must communicate with the backend through HTTP API contracts only.

### 2. Do not call HttpClient directly from pages

Razor pages must not create or configure `HttpClient` directly.

The expected call chain is:

```text id="d5ail4"
Razor page
  -> DispatcherBackendApiClient
  -> DispatcherBackendHttpClient
  -> HttpClient
  -> dispatcher-http-server
```

This keeps HTTP logic, error handling and endpoint configuration outside UI components.

### 3. Keep API DTOs explicit

Frontend API DTOs are stored in:

```text id="wk5wop"
frontend/Dispatcher.Web/Api/Models
```

Current DTO examples:

```text id="gaz43w"
HealthResponseDto
ReadinessResponseDto
RuntimeResponseDto
AlarmsResponseDto
```

For now, frontend DTOs are manually maintained. This is acceptable because the backend API is still evolving.

A generated client or OpenAPI-based workflow can be introduced later when the HTTP API becomes stable.

### 4. Treat backend unavailability as normal

The frontend must not crash when the backend is unavailable.

Expected behavior when the backend is down:

```text id="bcozo1"
Show an error alert.
Keep the page usable.
Keep navigation working.
Allow the operator to retry with Reload.
Do not throw unhandled UI exceptions.
```

Current support:

```text id="casm4f"
ApiCallResult<T>
Reload buttons
Error alerts
Safe nullable rendering
```

### 5. Prefer operational clarity over visual complexity

The frontend is an industrial operator console, not a marketing site.

The UI should prioritize:

```text id="upjd3i"
fast status recognition
clear alarm visibility
minimal navigation friction
stable layouts
readable typography
explicit error states
safe operator actions
```

Visual styling should support operations, not distract from them.

## UI technology strategy

The frontend uses MudBlazor as the primary UI component library.

Primary components:

```text id="qq5t1h"
MudLayout
MudAppBar
MudDrawer
MudNavMenu
MudCard
MudTable
MudAlert
MudChip
MudButton
MudProgressCircular
MudDialog
MudSnackbar
```

General UI rules:

```text id="czwawa"
Use cards for summary/status blocks.
Use tables for lists and operational records.
Use chips for compact statuses.
Use alerts for important system state and errors.
Use dialogs for confirmation before operator actions.
Use snackbars for short action feedback.
Use clear colors for status and severity.
```

## Current pages

The current foundation includes:

```text id="uw8sv3"
Dashboard
Health
Runtime
Alarms
API Diagnostics
```

### Dashboard

Purpose:

```text id="bxsieg"
Provide a high-level operator overview.
```

Current data:

```text id="yhzzq3"
Health status
Runtime status
Active alarm count
Backend URL/configuration
```

### Health

Purpose:

```text id="guyz1f"
Show backend health and readiness state.
```

Current data:

```text id="da8s7f"
GET /health
GET /ready
```

### Runtime

Purpose:

```text id="lfq5sb"
Show backend runtime endpoint state.
```

Current data:

```text id="nsqjaj"
GET /api/v1/runtime
```

### Alarms

Purpose:

```text id="ueo5z8"
Show active alarm overview.
```

Current data:

```text id="lal8v7"
GET /api/v1/alarms
```

### API Diagnostics

Purpose:

```text id="g6shkj"
Show frontend API client configuration and backend endpoint URLs.
```

This page is mostly for local development and troubleshooting.

## Future frontend page roadmap

The following pages are expected candidates for future frontend development.

This list is not final. It must be approved before implementation.

### Candidate operator pages

```text id="tzf62p"
Dashboard
Alarm Summary
Alarm Detail
Runtime
Telemetry Points
Devices
Trends / History
Configuration
Notifications
Operators
Audit Log
Settings
Diagnostics
```

### Candidate administrative pages

```text id="nrtck6"
Import / Export Configuration
Alarm Rules
Notification Routes
Operator Accounts
Roles and Permissions
Storage Status
Service Status
```

### Candidate diagnostic pages

```text id="sly27i"
API Diagnostics
Backend Health
Telemetry Adapter Status
Modbus Adapter Status
Storage Diagnostics
Notification Delivery Diagnostics
```

## UX agreement rule

Before starting real interface development for a new frontend feature, page group or operator workflow, the implementation must be preceded by a UX agreement step.

The UX agreement step must ask explicit questions and offer concrete answer options.

Implementation should not begin until the selected direction is clear.

### When UX agreement is required

A UX agreement step is required before:

```text id="vl6u5p"
Adding a new main page.
Changing the main navigation structure.
Designing the alarm workflow.
Designing operator actions.
Designing configuration editing screens.
Designing history/trend screens.
Designing authentication or operator account screens.
Changing the primary layout or page hierarchy.
Introducing production-oriented visual design.
```

### When UX agreement is not required

A full UX agreement step is not required for:

```text id="vj8lsh"
Small bug fixes.
Build fixes.
CORS fixes.
DTO adjustments.
API client plumbing.
Documentation-only changes.
Internal refactoring that does not change UI behavior.
```

## UX agreement question format

Before a UI-focused implementation step, the assistant must ask questions in this style.

### 1. Page scope

Question:

```text id="ar17k6"
Which pages should be included in this frontend increment?
```

Example answer options:

```text id="d978dz"
A. Dashboard only
B. Dashboard + Alarms
C. Dashboard + Alarms + Runtime
D. Full operator console section
E. Custom selection
```

### 2. Layout style

Question:

```text id="dk5vdg"
Which layout style should we use?
```

Example answer options:

```text id="n83cv2"
A. Dense industrial layout
B. Modern dashboard layout
C. Split overview/detail layout
D. Minimal diagnostic layout
E. Custom layout
```

### 3. Navigation model

Question:

```text id="du4v0u"
How should navigation be organized?
```

Example answer options:

```text id="vxzij2"
A. Single left drawer
B. Top navigation only
C. Left drawer grouped by Operations / Admin / Diagnostics
D. Dashboard-first with secondary tabs
E. Custom navigation
```

### 4. Alarm workflow

Question:

```text id="rjr64f"
What alarm workflow should the first version support?
```

Example answer options:

```text id="e5h1rb"
A. Read-only alarm list
B. Alarm list + detail panel
C. Alarm list + acknowledge action
D. Alarm list + acknowledge + shelve
E. Custom workflow
```

### 5. Data refresh behavior

Question:

```text id="dsbmps"
How should frontend data refresh work?
```

Example answer options:

```text id="n2x8z3"
A. Manual Reload only
B. Auto-refresh every 5 seconds
C. Auto-refresh every 10 seconds
D. Auto-refresh with pause/resume
E. WebSocket/SSE later, manual for now
```

### 6. Visual severity model

Question:

```text id="ayzfim"
How should severity be represented visually?
```

Example answer options:

```text id="kkhaz8"
A. Text only
B. Color chips only
C. Color chips + icons
D. Row highlighting
E. Row highlighting + chips + icons
```

### 7. Operator action safety

Question:

```text id="e7czvk"
How strict should confirmations be for operator actions?
```

Example answer options:

```text id="i72j7h"
A. No confirmations in early development
B. Confirm only destructive actions
C. Confirm all operator actions
D. Confirm all actions and require reason/comment
E. Custom policy
```

### 8. Target density

Question:

```text id="xerqrd"
How dense should the UI be?
```

Example answer options:

```text id="jctgj5"
A. Compact, SCADA-like
B. Balanced desktop layout
C. Spacious modern layout
D. Touch-friendly layout
E. Separate desktop and touch modes later
```

## Default recommendation for the first real UI increment

The recommended first real UI increment is:

```text id="rthsm2"
Dashboard + Alarm Summary + Runtime overview
```

Recommended defaults:

```text id="xf99us"
Layout: left drawer grouped by Operations / Admin / Diagnostics
Density: balanced desktop layout
Refresh: manual Reload first, auto-refresh later
Alarm workflow: read-only list first
Severity display: color chips + icons
Operator actions: no actions until backend actions are stable
```

Reasoning:

```text id="q6skvn"
The backend API is still maturing.
Alarm actions require careful backend contracts.
Read-only operational visibility is safer for the first real UI increment.
Dashboard and alarm visibility provide the highest operator value early.
```

## Frontend development phases

### Phase 1 — Foundation

Status: current stage.

Scope:

```text id="fav3v6"
Blazor WebAssembly application
MudBlazor shell
Backend URL configuration
Typed API client
Basic pages
CORS support
Local development documentation
```

### Phase 2 — Backend integration

Expected next frontend-facing work.

Scope:

```text id="lzv8rl"
Expand backend HTTP API contracts.
Replace skeleton runtime/alarm data with real data.
Improve API error models.
Add loading/error/empty states consistently.
Prepare frontend services for richer pages.
```

### Phase 3 — Operator console design

Scope requires UX agreement before implementation.

Expected questions:

```text id="j1hwv8"
Which pages are required?
Which navigation structure should be used?
Which dashboard widgets are required?
How should alarms be shown?
Which operator actions are allowed?
How should data refresh work?
```

### Phase 4 — Alarm operations

Scope requires UX agreement before implementation.

Potential features:

```text id="et2lka"
Alarm detail page
Alarm acknowledgement
Alarm shelving
Alarm suppression state
Alarm comments
Operator identity display
Audit trail integration
```

### Phase 5 — Runtime and telemetry views

Scope requires UX agreement before implementation.

Potential features:

```text id="tdfly4"
Device list
Tag list
Telemetry values
Adapter status
Runtime event stream
History/trend views
```

### Phase 6 — Configuration and administration

Scope requires UX agreement before implementation.

Potential features:

```text id="eufzco"
Configuration import/export
Alarm rule editor
Notification route editor
Operator management
Role management
System settings
```

## Definition of done for frontend UI increments

A frontend UI increment is done when:

```text id="o1j4td"
The intended pages are implemented.
The layout matches the agreed direction.
The API calls are routed through DispatcherBackendApiClient.
Loading state is visible.
Error state is visible.
Empty state is visible where relevant.
Backend unavailable state is handled.
dotnet build passes.
Backend tests pass.
Manual browser verification passes.
Documentation or release notes are updated.
```

## Current accepted limitations

The current frontend foundation intentionally does not include:

```text id="rqw12o"
Production UI design.
Real alarm operations.
Authentication UI.
Operator login flow.
Role-based UI.
Trend charts.
Configuration editing.
Frontend automated tests.
Deployment packaging.
```

These limitations are acceptable for the `v1.2.0-blazor-frontend` foundation release.
