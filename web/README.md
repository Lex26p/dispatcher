# Web Shell

`web/` is the React + TypeScript frontend of Dispatcher.

## Current stage

`CORE-003 — Web Shell` and `CORE-004 — Project Manager` are complete.

`CORE-005 — Users & Access` is in progress:

- Steps 1–5 established the Users & Access model/session boundary and backend-authoritative Project Manager authorization;
- Step 6A completed the real `users-access.v1` administration backend in commit `04e83879c73e298d1eac61acbd8e861f0ba5988d`;
- current Step 6B adds browser session ownership, login/logout/current user, authenticated Project Manager requests and the minimal Users & Access administration UI;
- Step 6C will complete local security-audit semantics for administration mutations before control mode in Step 7.

Notifications, messages, Dashboard and later service screens are not implemented yet.

## Toolchain baseline

- Node.js 24.20.0 LTS;
- npm 11.19.0;
- React 19.2.8;
- Vite 8.1.x;
- TypeScript 6.0.3;
- Vitest 4.1.10;
- React Testing Library;
- Playwright 1.62.1 with Chromium.

Direct package versions are pinned in `package.json`. Step 6B adds no frontend dependency.

## Local Web workflow

Web development and Web checks run natively on Windows. C++ backend checks remain Linux/WSL.

Use the Windows command shims from PowerShell:

    npm.cmd ci
    npm.cmd run typecheck
    npm.cmd run test
    npm.cmd run test:e2e

Install Chromium when needed:

    npx.cmd playwright install chromium

Development server:

    npm.cmd run dev -- --host 0.0.0.0

## Navigation and authentication

The current real shell destinations are:

- `Рабочая область` → `/`, public;
- `Вход` → `/login`, available while unauthenticated;
- `Проекты` → `/projects`, available to an authenticated user;
- `Пользователи и доступ` → `/access`, shown only for an authenticated user with authoritative global `admin` capability.

Direct navigation does not become a security boundary. An unauthenticated `/projects` or `/access` route renders the login gate. A non-admin authenticated user opening `/access` receives an explicit insufficient-rights state. Backend providers still make every authorization decision.

No router dependency is required yet; the shell continues to use the browser History API.

## Browser session boundary

Step 6B introduces `src/user-session/BrowserSessionTransport.ts` and `UserSessionProvider.tsx`.

The browser policy is intentionally small:

- the raw opaque session bearer is stored only in `sessionStorage` for the current browser session;
- storage key: `dispatcher.user-session.v1`;
- `localStorage` is not used;
- the browser does not persist a user ID, roles or permissions as security authority;
- reload with a stored bearer performs authoritative `users-access.v1/current-session` restoration;
- `auth.invalid_session` and `auth.session_expired` clear the local bearer and authenticated React state;
- a temporary Hub/provider/transport failure does not itself revoke the durable server-side session and can be retried;
- logout attempts server-side invalidation and always clears local user-sensitive state.

`BrowserSessionServiceHubClient` wraps the already-shared `ServiceHubClient`. It does not create another WebSocket. Public `users-access.v1/login` remains unauthenticated; protected requests automatically receive the existing Service Hub `auth: {type:"session", token}` field.

The low-level Service Hub protocol and `ServiceHubClient` remain unchanged.

## Current user

`UserSessionProvider` exposes explicit states:

- `unauthenticated`;
- `restoring`;
- `authenticated`.

The global Header shows a compact login action or current-user summary/logout action. After authoritative session restoration, Web asks `evaluate-access` for global `admin`; returned effective capabilities are used for presentation/navigation only. They do not replace backend authorization.

## Project Manager Web path

`src/project-manager/ProjectManagerClient.ts` remains a typed adapter over the shared Service Hub transport and still knows only the Project Manager v1 business contract.

Because the shared transport is now session-aware, existing Project Manager requests receive the current bearer automatically without adding user ID, roles, permissions or auth token to Project Manager business payloads.

`/projects` is login-gated in Web, while the production Project Manager still independently requires/authorizes the session server-side.

## Project context

`src/project-context/ProjectContextProvider.tsx` still stores only a Project snapshot for navigation in `sessionStorage`; it is never proof of access.

Step 6B reconciles it with the user lifecycle:

- no authenticated user → selected project is cleared;
- logout → project context is cleared;
- authenticated user change → old project context is cleared;
- reload waits for user-session restoration before authoritative project revalidation;
- `project.not_found` or `access.forbidden` during revalidation clears the selected project;
- temporary Hub/provider failures keep the navigation snapshot rather than treating an outage as proof of revoked access.

## Users & Access client and administration UI

`src/users-access/UsersAccessClient.ts` is a typed adapter for the existing `users-access.v1` contract. It covers session-core and Step 6A administration operations and validates successful response shapes before exposing data to React.

`/access` provides the minimal administration UI required by CORE-005:

- list/create users;
- enable/disable users;
- set/reset password using the 15..1024-byte backend baseline;
- list/create permission sets with independent `view`, `control`, `edit`, `admin` capabilities;
- list/add/remove global/project assignments.

A project-scoped assignment takes an explicit project ID. The UI does not assume global `admin` implicitly grants project `view`, because capabilities are independent and the backend remains authoritative.

Device/Dashboard-specific ACL, groups/ABAC, external identity providers and control mode are outside Step 6B.

## Service Hub URL configuration

The shared client uses `VITE_SERVICE_HUB_URL` when set. Example:

    $env:VITE_SERVICE_HUB_URL = "ws://127.0.0.1:8090/v1/ws"
    npm.cmd run dev -- --host 0.0.0.0

Without it, Web derives `/v1/ws` from the page origin and chooses `ws://` or `wss://` from the page protocol.

Automatic reconnect remains outside the current Web baseline. The shell remains rendered when Service Hub is unavailable.

## Browser checks

Backend-independent smoke:

    npm.cmd run test:e2e

Generic real Service Hub regression:

    npm.cmd run test:e2e:service-hub

The generic integration still exercises the same shared client API. The Step 6B session-aware wrapper is transparent for unrelated public test services.

Current Project Manager browser integration:

    npm.cmd run test:e2e:project-manager

That existing runner starts real Service Hub + Project Manager but intentionally does not start Users & Access yet. In Step 6B it verifies that the real connected shell gates `/projects` behind login instead of surfacing raw `auth.invalid_session` as normal UI.

The full authenticated multi-process browser security path — `Browser → Service Hub → Users & Access → Project Manager` with allowed/denied permissions, revocation and restart behavior — remains the planned Step 7 integration/acceptance work.
