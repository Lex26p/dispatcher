# Web Shell

`web/` is the React + TypeScript frontend of Dispatcher.

## Current stage

`CORE-003 / Step 1` established the frontend build/test skeleton.

`CORE-003 / Step 2` established the compact global Header and workspace layout.

`CORE-003 / Step 3` established the first real global navigation behavior without adding a router dependency. The menu exposes only the current Web Shell workspace, uses the browser History API, provides an unknown-route fallback and supports predictable keyboard interaction.

`CORE-003 / Step 4` established a reusable TypeScript Service Hub v1 client. It is independent of React and covers the direct browser WebSocket boundary, connection state, request correlation, cancellation, Hub request errors and transport failures.

`CORE-003 / Step 5` established shared React ownership of that client: one provider controls connect/disconnect lifecycle, exposes the client and connection state through a hook, and shows a compact Service Hub connection indicator in the global Header. The shell remains usable when Service Hub is unavailable.

`CORE-003 / Step 6` established the real browser/backend integration path against the existing C++ Service Hub and an automation-only test provider.

`CORE-003 — Web Shell` is complete. `CORE-004 — Project Manager` uses that frontend foundation.

`CORE-004 / Step 4` established the first real service UI: the `/projects` destination, typed Project Manager client adapter, project list and create/edit form.

`CORE-004 / Step 5` established shared project context for the Web Shell: selected Project or explicit global mode, current-session persistence, remote validation and compact Header indication.

`CORE-004 / Step 6` established a separate real Project Manager browser integration path with durable restart recovery.

`CORE-004 — Project Manager` is complete.

`CORE-005 — Users & Access` is in progress. Steps 1–4 established Users & Access domain/persistence/session semantics and optional per-request Service Hub session auth.

`CORE-005 / Step 5` now protects the real Project Manager backend. The existing Web Shell still has no owned logged-in user session, so unauthenticated `/projects` requests are intentionally rejected by the backend with `auth.invalid_session`.

Web login/current-user/session restoration and Users & Access administration remain Step 6. The temporary Step 5 browser state is therefore fail-closed rather than continuing the old unauthenticated CORE-004 CRUD path.

Notifications, messages, Dashboard and later service screens are not implemented yet.

## Toolchain baseline

- Node.js 24.20.0 LTS;
- npm 11.19.0;
- React 19.2.8;
- Vite 8.1.x;
- TypeScript 6.0.3;
- Vitest 4.1.10;
- React Testing Library;
- Playwright 1.62.1 with Chromium for browser smoke/integration tests.

Direct package versions are pinned exactly in `package.json`.

## Local Web workflow

Web development and automated Web checks are run natively on Windows. The C++ backend keeps its Linux/WSL workflow.

Use `npm.cmd` and `npx.cmd` from PowerShell so the workflow does not depend on the PowerShell script execution policy.

Clean installs use the committed lockfile:

    npm.cmd ci

Install the Chromium binary used by the browser tests:

    npx.cmd playwright install chromium

## Development server

    npm.cmd run dev -- --host 0.0.0.0

Vite prints the local development URL.

## Checks

TypeScript:

    npm.cmd run typecheck

Unit/component tests:

    npm.cmd run test

Production build:

    npm.cmd run build

Browser smoke/integration against a fresh production build:

    npm.cmd run test:e2e

`test:e2e` builds the production frontend before starting Playwright.

Combined unit + production build check:

    npm.cmd run check

## Current navigation

The shell currently has two real destinations:

- `Рабочая область` → `/`;
- `Проекты` → `/projects`.

The menu opens from the compact global Header, marks the active route with `aria-current`, moves keyboard focus into navigation, closes with `Escape`, and closes after navigation.

Unknown paths render a Web Shell fallback with a way back to `/`.

No router dependency is required yet. The project list/editor uses local component state inside `/projects`; selected project context is shared separately through `ProjectContextProvider` and survives ordinary shell navigation.

## Project Manager Web client

`src/project-manager/ProjectManagerClient.ts` is a typed adapter over the shared Service Hub client. It uses service address `project-manager.v1` and the four v1 operations from `docs/architecture/project-manager-contract.md`:

- `create-project`;
- `list-projects`;
- `get-project`;
- `update-project`.

It does not open another WebSocket and it validates successful response payload shape before exposing `Project` data to React.

`src/project-manager/ProjectManagerView.tsx` implements the current list/editor UI. It provides loading, empty and local error states, creation and editing of `name`/`description`, and keeps the rest of Web Shell usable if Service Hub or Project Manager is unavailable.

The Web view does not yet own session auth. Step 5 deliberately leaves that UX for Step 6, while the backend already enforces authentication and authorization. Until Step 6, real Project Manager requests from this view fail with `auth.invalid_session`.

## Project context

`src/project-context/ProjectContextProvider.tsx` provides the shared frontend project context. The value is either a real Project v1 snapshot (`id`, `name`, `description`) or `null` for explicit global mode.

The context:

- uses the existing shared Service Hub client through `ProjectManagerClient`;
- persists the selected project in `sessionStorage` for the current browser session only;
- restores and refreshes a stored project with `get-project` when Service Hub is connected;
- clears the selection only when Project Manager confirms `project.not_found`;
- keeps the selection during temporary Hub/provider/timeout failures;
- can be cleared explicitly from the compact global Header;
- can be selected from real Project Manager list data in `/projects`;
- updates its display snapshot if the selected project is renamed in the editor.

No project field is added to the Service Hub envelope. The current `sessionStorage` project snapshot is frontend navigation context, not proof of access. `CORE-005 / Step 6` must reconcile this context with the authenticated user/session lifecycle and clear/revalidate it when user/access changes.

## Service Hub client

`src/service-hub/ServiceHubClient.ts` implements the browser-facing Service Hub v1 client without React dependencies.

The client:

- accepts an explicit WebSocket URL;
- requests subprotocol `dispatcher.service-hub.v1`;
- exposes connection state and state subscription;
- supports parallel request/response correlation;
- returns a request handle with `id`, `response` Promise and `cancel()`;
- distinguishes `ServiceHubRequestError`, `ServiceHubTransportError` and connection-level `ServiceHubProtocolError`;
- supports optional per-request Service Hub `auth` context for the session credential shape established in `CORE-005 / Step 4`;
- does not own persistent browser session-token storage, logged-in user state, automatic reconnect or an application heartbeat.

React integration is provided by `src/service-hub/ServiceHubProvider.tsx`. `useServiceHub()` exposes the shared client and current connection state to future Web screens.

## Authenticated request transport

`CORE-005 / Step 4` keeps authentication on the same Service Hub WebSocket boundary.

A request may include an optional transport context equivalent to:

    {
      type: "session",
      token: "64-lowercase-hex-characters"
    }

The browser client accepts this context per request and places it in the Service Hub `auth` field separately from business `payload`.

Important boundaries:

- public `users-access.v1/login` is sent without auth;
- the client does not convert a token into `user_id`, roles or permissions;
- the presence of syntactically valid auth does not prove that the session is valid;
- authoritative validation belongs to Users & Access / protected backend providers;
- persistent browser token storage, restoration and shared authenticated user context are intentionally deferred to `CORE-005 / Step 6`.

Step 5 uses the same transport field to protect Project Manager. No Project Manager business payload receives identity or permission fields.

## Service Hub URL configuration

The shared client uses `VITE_SERVICE_HUB_URL` when it is set. Example for local development:

    $env:VITE_SERVICE_HUB_URL = "ws://127.0.0.1:8090/v1/ws"
    npm.cmd run dev -- --host 0.0.0.0

When `VITE_SERVICE_HUB_URL` is not set, Web Shell derives the URL from the page origin and uses `/v1/ws`, choosing `ws://` for HTTP pages and `wss://` for HTTPS pages.

Web Shell does not implement automatic reconnect. A failed or closed connection returns the shared client to its disconnected state while the React application stays usable; reconnect is explicit.

## Real Service Hub browser integration

The normal browser smoke remains backend-independent:

    npm.cmd run test:e2e

The real Service Hub integration check is separate:

    npm.cmd run test:e2e:service-hub

This integration command is intended for the Windows + WSL development workflow. It:

- incrementally configures/builds the existing C++ `dispatcher_service_hub` target in WSL;
- starts the real Service Hub on a temporary loopback port;
- starts an automation-only Node.js test provider registered as `test.web-shell`;
- builds the production Web Shell with an explicit `VITE_SERVICE_HUB_URL`;
- runs Playwright against the real shared `ServiceHubClient`;
- verifies success, parallel response correlation, unknown-service error, cancel propagation, real Hub shutdown, disconnected state, explicit reconnect and a new request after reconnect;
- stops the temporary Service Hub after the check.

The Node.js provider is test support only. It is not a production backend service and does not change the platform transport contract.

The integration build enables `VITE_SERVICE_HUB_E2E=1` only to expose the already-created shared client to Playwright. Normal production builds do not expose this test seam.

## Real Project Manager browser integration

The Project Manager end-to-end check remains separate from backend-independent smoke and generic Service Hub integration:

    npm.cmd run test:e2e:project-manager

During `CORE-005 / Step 5` this browser check has a deliberately narrower purpose than the completed CORE-004 acceptance.

The runner still starts real C++ Service Hub and Project Manager processes and serves a production Web build. Because Step 6 login/session ownership is not implemented yet, the browser does **not** attempt to bypass the new backend security boundary with a test-only user injection.

Instead the Step 5 browser assertion verifies:

- Service Hub connects normally;
- `/projects` remains a real Project Manager destination;
- unauthenticated `list-projects` is rejected by the production Project Manager with `auth.invalid_session`;
- an unauthenticated create attempt is also rejected;
- the Web Shell remains rendered rather than treating UI visibility as authorization.

Authenticated browser CRUD, user-aware project context and permission-sensitive controls return as part of Step 6/7 integration after the Web Shell has a production user/session context.

The stronger Step 5 backend authorization matrix is covered by `project-manager.service-hub-integration` in WSL, which runs real Service Hub + Project Manager + Users & Access processes.

This staging avoids introducing a hidden test-only browser authentication mechanism and keeps Web presentation separate from the actual security boundary.
