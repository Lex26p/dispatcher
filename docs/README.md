# Документация проекта «Диспетчер»

Этот файл — карта документации и основная точка входа.

## Правило источника истины

Репозиторий является источником истины проекта. Переписка используется для обсуждения, а согласованные решения фиксируются в документации.

В документы не следует записывать неподтверждённые решения как факт.

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

- [`README.md`](architecture/README.md) — архитектурный baseline: микросервисы, Hub-модель, Device Manager, модель метрик и выбранные языки/инструменты.
- [`data-hub-contract.md`](architecture/data-hub-contract.md) — внешний gRPC + Protocol Buffers контракт Data Hub и его runtime-семантика.
- [`service-hub-contract.md`](architecture/service-hub-contract.md) — внешний WebSocket + JSON контракт Service Hub, provider registration и request/response semantics.

Технические решения добавляются по мере реальной необходимости. Для Data Hub и Service Hub транспорт и сериализация уже зафиксированы; технологии Event Hub, БД и deployment выбираются в соответствующих будущих спринтах.

### `docs/development/`

Рабочий план разработки и отчёты по завершённым спринтам.

- [`ROADMAP.md`](development/ROADMAP.md) — этапы, спринты и текущая точка разработки.
- [`sprints/CORE-001.md`](development/sprints/CORE-001.md) — план и итоговый отчёт завершённого спринта Data Hub.
- [`sprints/CORE-002.md`](development/sprints/CORE-002.md) — план и итоговый отчёт завершённого спринта Service Hub.
- [`sprints/CORE-003.md`](development/sprints/CORE-003.md) — план и итоговый отчёт завершённого спринта Web Shell.

Следующий спринт — `CORE-004 — Project Manager`; его подробный sprint plan создаётся перед началом реализации.

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
