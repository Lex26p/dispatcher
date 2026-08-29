# Диспетчер

**Диспетчер** — универсальная модульная платформа диспетчеризации и управления оборудованием.

Проект строится вокруг небольшого ядра и устанавливаемых расширений. Ядро предоставляет универсальные механизмы работы с проектами, Dashboard, мнемосхемами, устройствами, метриками, событиями, пользователями и пакетами. Управление оборудованием выполняется через метрики, для которых разрешена запись. Дополнительные возможности — автоматизация, история, CCTV, СКУД, дополнительные протокольные драйверы и другие предметные функции — подключаются как плагины. Базовые Modbus и SNMP разрабатываются вместе с ядром, чтобы проверить универсальность драйверной архитектуры на реальном оборудовании.

## Текущий этап

Идёт этап `L1-01 — Ядро платформы`.

`CORE-001 — Data Hub` завершён: существует отдельный C++20-сервис с gRPC + Protocol Buffers proto3 контрактом, хранением текущих значений, retained/live подписками, общей моделью state-метрик, базовым путём `WriteMetric` и Linux lifecycle-поведением.

`CORE-002 — Service Hub` завершён: существует отдельный C++20-сервис с WebSocket + UTF-8 JSON v1 контрактом, provider registration, адресным request/response routing, параллельной correlation, timeout/cancel, disconnect/reconnect semantics и напрямую browser-compatible клиентской границей.

`CORE-003 — Web Shell` завершён: существует самостоятельный React + TypeScript Web Shell с компактным global Header, глобальной навигацией, рабочей областью, общим browser-side Service Hub client/React connection boundary и реальной browser → Service Hub → test provider интеграционной проверкой.

`CORE-004 — Project Manager` завершён: существует отдельный C++20 Project Manager со стабильными project ID, локальным SQLite schema v1, versioned Service Hub provider `project-manager.v1`, Web-разделом `/projects`, общим browser-session project context и реальной browser → Service Hub → Project Manager → SQLite проверкой с restart recovery. Authentication/authorization по-прежнему относится к `CORE-005 — Users & Access`.

Следующий спринт — `CORE-005 — Users & Access`. Его реализация начинается только после отдельного sprint plan commit на проверенном closure baseline `CORE-004`.

## Документация

Начальная точка: [`docs/README.md`](docs/README.md).

Архитектура: [`docs/architecture/README.md`](docs/architecture/README.md).

Контракт Data Hub: [`docs/architecture/data-hub-contract.md`](docs/architecture/data-hub-contract.md).

Контракт Service Hub: [`docs/architecture/service-hub-contract.md`](docs/architecture/service-hub-contract.md).

Контракт Project Manager: [`docs/architecture/project-manager-contract.md`](docs/architecture/project-manager-contract.md).

Дорожная карта: [`docs/development/ROADMAP.md`](docs/development/ROADMAP.md).

Итог `CORE-001`: [`docs/development/sprints/CORE-001.md`](docs/development/sprints/CORE-001.md).

Итог `CORE-002`: [`docs/development/sprints/CORE-002.md`](docs/development/sprints/CORE-002.md).

Итог `CORE-003`: [`docs/development/sprints/CORE-003.md`](docs/development/sprints/CORE-003.md).

Итог `CORE-004`: [`docs/development/sprints/CORE-004.md`](docs/development/sprints/CORE-004.md).

Контекст текущего обсуждения для продолжения работы в другом чате: [`docs/context/CHAT_CONTEXT.md`](docs/context/CHAT_CONTEXT.md).
