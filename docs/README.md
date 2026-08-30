# Документация проекта «Диспетчер»

Этот файл — карта документации и основная точка входа.

## Правило источника истины

Репозиторий является источником истины проекта. Переписка используется для обсуждения, а согласованные решения фиксируются в документации.

В документы не следует записывать неподтверждённые решения как факт.

Документация ведётся вместе с разработкой. Если конкретный шаг меняет уже документированные факты, контракт, выбранную технологию, команды, статус или важную границу, соответствующий документ обновляется в этом же шаге. Финальный documentation audit в конце спринта проверяет целостность и рассинхронизацию между документами, но не заменяет документирование решений по мере их принятия.

Обязательные правила рабочего процесса AI-агента находятся в корневом [`AGENTS.md`](../AGENTS.md).

## Структура

### `docs/concept/`

Согласованная концепция ядра: **что представляет собой продукт и как он должен работать с точки зрения пользователя**, без детальной реализации.

- [`00-overview.md`](concept/00-overview.md) — общий обзор продукта.
- [`01-core-principles.md`](concept/01-core-principles.md) — общие принципы ядра.
- [`02-projects.md`](concept/02-projects.md) — проекты.
- [`03-dashboard.md`](concept/03-dashboard.md) — Dashboard.
- [`04-mimics.md`](concept/04-mimics.md) — мнемосхемы.
- [`05-devices-metrics.md`](concept/05-devices-metrics.md) — устройства, метрики, запись и состояния.
- [`06-event-manager.md`](concept/06-event-manager.md) — диспетчер событий.
- [`07-users-access.md`](concept/07-users-access.md) — пользователи и права.
- [`08-system-administration.md`](concept/08-system-administration.md) — системные данные и администрирование.
- [`09-package-manager.md`](concept/09-package-manager.md) — менеджер пакетов.
- [`10-web-ui.md`](concept/10-web-ui.md) — Web UI.
- [`11-core-boundaries.md`](concept/11-core-boundaries.md) — границы ядра.

### `docs/plugins/`

Концепции дополнительных модулей.

- [`automation/concept.md`](plugins/automation/concept.md) — автоматизация, JavaScript и планировщик.

### `docs/architecture/`

Согласованная архитектура системы на текущем уровне детализации.

- [`README.md`](architecture/README.md) — архитектурный baseline: микросервисы, Hub-модель, границы основных сервисов, модель метрик и выбранные языки/инструменты.
- [`data-hub-contract.md`](architecture/data-hub-contract.md) — внешний gRPC + Protocol Buffers контракт Data Hub и его runtime-семантика.
- [`service-hub-contract.md`](architecture/service-hub-contract.md) — внешний WebSocket + JSON контракт Service Hub, provider registration, request/response и authenticated request transport semantics.
- [`project-manager-contract.md`](architecture/project-manager-contract.md) — versioned Project Manager v1 operations/payload/errors поверх Service Hub.
- [`users-access-contract.md`](architecture/users-access-contract.md) — versioned Users & Access v1 authentication/session/access payload contract и session semantics.

Технические решения добавляются по мере реальной необходимости. Для Data Hub и Service Hub transport/serialization уже зафиксированы; Project Manager использует versioned `project-manager.v1` поверх Service Hub и локальный SQLite schema v1. Users & Access использует локальный SQLite storage и versioned `users-access.v1` payload contract. `CORE-005 / Step 4` добавил optional session-auth context в существующий Service Hub v1 и production `users-access.v1` provider без второго transport; `CORE-005 / Step 5` применил эту boundary к Project Manager с backend-authoritative access evaluation и fail-closed semantics. Service-local SQLite не является выбором общей БД платформы. Event Hub, persistence-стратегия будущих сервисов и deployment выбираются в соответствующих будущих спринтах.

### `docs/development/`

Рабочий план разработки и отчёты по спринтам.

Roadmap использует три уровня: этап → спринт → шаг. Этапы задают долгосрочное направление; спринты конкретного этапа определяются перед началом этапа; перед реализацией каждого нового спринта его план сначала раскладывается на локальные шаги и фиксируется в sprint-файле. Для текущего `L1-01` спринты уже определены.

Для текущей части `L1-01` дополнительно действует backend-first staging: до закрытия `CORE-013` новые Web features не разрабатываются вместе с backend-спринтами. React + TypeScript остаются выбранным stack; backend-спринты вместо раннего UI обязаны оставлять contracts и обновлять `WEB_IMPLEMENTATION.md`. После CORE-013 отдельный CORE-014 собирает готовый backend foundation в Web.

- [`ROADMAP.md`](development/ROADMAP.md) — этапы, спринты и текущая точка разработки.
- [`WEB_IMPLEMENTATION.md`](development/WEB_IMPLEMENTATION.md) — накопительный handoff/backlog будущей Web-интеграции во время backend-first фазы; обновляется backend-спринтами вместе с contracts.
- [`sprints/CORE-001.md`](development/sprints/CORE-001.md) — план и итоговый отчёт завершённого спринта Data Hub.
- [`sprints/CORE-002.md`](development/sprints/CORE-002.md) — план и итоговый отчёт завершённого спринта Service Hub.
- [`sprints/CORE-003.md`](development/sprints/CORE-003.md) — план и итоговый отчёт завершённого спринта Web Shell.
- [`sprints/CORE-004.md`](development/sprints/CORE-004.md) — план и итоговый отчёт завершённого спринта Project Manager.
- [`sprints/CORE-005.md`](development/sprints/CORE-005.md) — план и итоговый отчёт завершённого спринта Users & Access.

`CORE-004 — Project Manager` завершён closure commit `29b1f0ea750633cc53cc4e023585835d2b06ad8b`. `CORE-005 — Users & Access` завершён после backend Step 8 acceptance/documentation closure; итоговый отчёт находится в `sprints/CORE-005.md`. Следующая задача — планирование `CORE-006 — Device Manager`. Разработка продолжает идти backend-first через CORE-013, а Web context накапливается в `WEB_IMPLEMENTATION.md` до отдельного CORE-014.

### `docs/context/`

- [`CHAT_CONTEXT.md`](context/CHAT_CONTEXT.md) — контекст для продолжения работы в новом чате.

### `docs/decisions/`

Зарезервировано для будущих документов о фундаментальных решениях и причинах их принятия.

## Уровни документации

1. **Concept** — что представляет собой продукт и его сервисы.
2. **Plugins** — расширения продукта.
3. **Architecture** — как система устроена технически на согласованном уровне.
4. **Decisions** — почему приняты ключевые решения.

Документы не следует превращать в подробное ТЗ раньше соответствующего этапа.
