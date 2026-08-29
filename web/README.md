# Web Shell

`web/` is the React + TypeScript frontend of Dispatcher.

## Current stage

`CORE-003 / Step 1` established the frontend build/test skeleton.

`CORE-003 / Step 2` established the compact global Header and workspace layout.

`CORE-003 / Step 3` adds the first real global navigation behavior without adding a router dependency. The menu exposes only the current Web Shell workspace, uses the browser History API, provides an unknown-route fallback and supports predictable keyboard interaction.

Notifications, messages, user actions, Service Hub client, authentication, Dashboard and future service screens are not implemented yet.

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

## Dependency install

Clean installs use the committed lockfile:

    npm ci

## Browser test dependency

Install the Chromium binary and Linux dependencies used by the browser tests:

    npx playwright install --with-deps chromium

## Development server

    npm run dev -- --host 0.0.0.0

Vite prints the local development URL.

## Checks

TypeScript:

    npm run typecheck

Unit/component test:

    npm run test

Production build:

    npm run build

Browser smoke/integration against a fresh production build:

    npm run test:e2e

Combined unit + production build check:

    npm run check

## Current navigation

The current shell route is `/`.

The global menu currently contains exactly one real destination:

- `Рабочая область` → `/`.

The menu opens from the compact global Header, marks the active route with `aria-current`, moves keyboard focus into navigation, closes with `Escape`, and closes after navigation.

Unknown paths render a Web Shell fallback with a way back to `/`.

Future service links are intentionally absent until those services exist. `CORE-003 / Step 4` adds the reusable TypeScript Service Hub client; it does not add new navigation destinations by itself.
