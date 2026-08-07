# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA перед этим пакетом:** `fa38f437a90f98cdb4091a25187eec67f2213e6a`.

## 1. Завершённый Product Concept

Общая горизонтальная Product Concept завершена и остаётся фундаментом требований:

- `PRD-Q001–PRD-Q803`;
- Full / Compact / Edge;
- общая object/configuration/command/alarm/history/package/security модель;
- application scope от одного Compact-контроллера до крупной распределённой Full/Edge SCADA.

Product Concept не расширяется по инерции. Возврат туда происходит только при обнаружении нового фундаментального product-level решения или противоречия.

## 2. Текущий этап — Functional Specification

Functional Specification определяет наблюдаемое поведение продукта, user flows, lifecycle, validations, permissions и Full/Compact/Edge semantics достаточно подробно для последующего UX/API/System Architecture design.

Рабочий навигатор: `../functional/ROADMAP.md`.

## 3. Engineering / Configuration — текущее состояние

**Статус:** `IN PROGRESS`.

Принято и подготовлено к `ENG-CP03`:

- `ENG-Q001–ENG-Q790` без намеренных пропусков;
- `ENG-FR001–ENG-FR150`;
- Engineering workspace/change-set/object foundation;
- Types / Device Profiles / Object Templates и их lifecycle/migrations;
- Connections / Adapters / Endpoints / Credentials / execution placement;
- connection testing/runtime/deploy/authority separation;
- полный Parameter semantic pipeline от source fact до effective value;
- types/quantities/units, source bindings, acquisition, timestamps, provenance, quality/freshness;
- normalization/calibration/limits, historization/late data/gaps/deadbands/retention;
- multiple sources, manual substitution, calculated/aggregate/counter semantics;
- Parameter bulk engineering, observed/runtime diagnostics, commissioning, permissions, scale, validation и impact.

Checkpoint history:

- `ENG-CP01` — `688392edb17ddce6e4d3874ff54344aacc2033b0`;
- `ENG-CP02` — `fa38f437a90f98cdb4091a25187eec67f2213e6a`;
- `ENG-CP03` добавляет `ENG-Q251–ENG-Q790` и `ENG-FR059–ENG-FR150` и ожидает commit/push.

## 4. Следующая точка продолжения

После подтверждения SHA `ENG-CP03` продолжить с:

> `ENG-Q791...` — Semantic Commands: definitions, arguments, risk, safety/preconditions, confirmations/approvals, execution lifecycle, feedback/success criteria, timeout/uncertainty, idempotency/retry, concurrency, Edge/offline policy и diagnostics.

Далее закрыть оставшиеся Engineering-блоки по `ROADMAP.md`.

## 5. Checkpoint discipline

Пользователю не требуется вручную отслеживать момент фиксации. Правила находятся в `../functional/ROADMAP.md`:

- смысловой блок имеет приоритет над фиксированным количеством вопросов;
- ориентир — `100–200` новых Q между checkpoints;
- перед ZIP обязательны перенос всех Q/FR, coverage-check, roadmap и PROJECT_STATE update;
- диапазон следующего checkpoint заранее не фиксируется.

## 6. Architecture readiness

До `GATE-A` не выбирать по инерции frontend/backend frameworks, конкретную DB, message broker, internal transport, fencing/lease/consensus implementation или исходный код.

`GATE-A` выполняется после достаточной детализации Engineering + Operations + Web Platform и проверяет центральный сквозной contour от configuration до runtime/command/audit.
