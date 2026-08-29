# Web Shell

`web/` is the React + TypeScript frontend of Dispatcher.

## Current stage

`CORE-003 / Step 1` established the frontend build/test skeleton.

`CORE-003 / Step 2` adds the first App Shell layout: a compact global Header, a reserved global-actions area and a workspace that keeps most of the viewport available to future service UIs.

The main-menu trigger is intentionally disabled until Step 3 adds real navigation. Notifications, messages, user actions, Service Hub client, authentication, Dashboard and future service screens are not implemented yet.

## Toolchain baseline

- Node.js 24.20.0 LTS;
- npm 11.19.0;
- React 19.2.8;
- Vite 8.1.x;
- TypeScript 6.0.3;
- Vitest 4.1.10;
- React Testing Library;
- Playwright 1.62.1 with Chromium for the initial browser smoke test.

Direct package versions are pinned exactly in `package.json`.

## First dependency bootstrap

The first install creates the repository lockfile:

    cd /mnt/c/Projects/dispatcher/web
    npm install --package-lock-only
    npm ci

`package-lock.json` must be committed with Step 1.

After the lockfile exists, normal clean installs use only:

    npm ci

## Browser test dependency

Install the Chromium binary and Linux dependencies used by the smoke test:

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

Browser smoke against the production preview:

    npm run test:e2e

Combined unit + production build check:

    npm run check

## Current page

The current UI is the base Dispatcher App Shell. It contains only the structural Header and workspace required by Step 2.

Navigation behavior starts in `CORE-003 / Step 3`; the current menu trigger is visible but disabled so the shell does not expose a false working action.
