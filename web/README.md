# Web Shell

`web/` is the React + TypeScript frontend of Dispatcher.

## Current stage

`CORE-003 / Step 1` established the frontend build/test skeleton.

`CORE-003 / Step 2` established the compact global Header and workspace layout.

`CORE-003 / Step 3` established the first real global navigation behavior without adding a router dependency. The menu exposes only the current Web Shell workspace, uses the browser History API, provides an unknown-route fallback and supports predictable keyboard interaction.

`CORE-003 / Step 4` established a reusable TypeScript Service Hub v1 client. It is independent of React and covers the direct browser WebSocket boundary, connection state, request correlation, cancellation, Hub request errors and transport failures.

`CORE-003 / Step 5` established shared React ownership of that client: one provider controls connect/disconnect lifecycle, exposes the client and connection state through a hook, and shows a compact Service Hub connection indicator in the global Header. The shell remains usable when Service Hub is unavailable.

`CORE-003 / Step 6` established the real browser/backend integration path against the existing C++ Service Hub and an automation-only test provider.

`CORE-003 — Web Shell` is complete. `CORE-004 — Project Manager` now uses that frontend foundation.

`CORE-004 / Step 4` established the first real service UI: the `/projects` destination, typed Project Manager client adapter, project list and create/edit form.

`CORE-004 / Step 5` established shared project context for the Web Shell: selected Project or explicit global mode, current-session persistence, remote validation and compact Header indication.

`CORE-004 / Step 6` adds a separate real Project Manager browser integration path with durable restart recovery.

Notifications, messages, user actions, authentication, Dashboard and later service screens are not implemented yet.

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

No project field is added to the Service Hub envelope, and no user-specific saved preference is introduced before `CORE-005`.

## Service Hub client

`src/service-hub/ServiceHubClient.ts` implements the browser-facing Service Hub v1 client without React dependencies.

The client:

- accepts an explicit WebSocket URL;
- requests subprotocol `dispatcher.service-hub.v1`;
- exposes connection state and state subscription;
- supports parallel request/response correlation;
- returns a request handle with `id`, `response` Promise and `cancel()`;
- distinguishes `ServiceHubRequestError`, `ServiceHubTransportError` and connection-level `ServiceHubProtocolError`;
- does not implement authentication, automatic reconnect or an application heartbeat.

React integration is provided by `src/service-hub/ServiceHubProvider.tsx`. `useServiceHub()` exposes the shared client and current connection state to future Web screens.

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

The Project Manager end-to-end check is separate from both backend-independent smoke and the generic Service Hub integration:

    npm.cmd run test:e2e:project-manager

When the repository npm baseline is invoked through `npx.cmd --yes npm@11.19.0`, the runner keeps its nested production build on the same npm executable.

The command is intended for the Windows + WSL workflow. It:

- builds the real C++ Service Hub and Project Manager targets in WSL;
- starts Service Hub on a temporary loopback port;
- starts Project Manager against a temporary SQLite database;
- waits until `project-manager.v1` is actually registered;
- builds and serves the production Web Shell against that Service Hub;
- creates and edits a project through the real browser UI;
- selects the real project as shared Web Shell context;
- performs parallel `list-projects` and `get-project` requests through the shared browser Service Hub client;
- stops only Project Manager while Service Hub remains connected and verifies the local unavailable-service UI without losing project context;
- restarts Project Manager with the same SQLite database and waits for provider re-registration;
- verifies the stable project ID, persisted data, restored session context and a new update after restart;
- stops the exact temporary processes and removes the temporary database.

This runner does not introduce a test provider for Project Manager: the provider is the production C++ `dispatcher-project-manager` process.

