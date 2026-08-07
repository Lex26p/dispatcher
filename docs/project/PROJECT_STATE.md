# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA перед этим пакетом:** `45756985f305ac0e952319d2b399262726beb964`.

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

Принято и подготовлено к `ENG-CP04`:

- `ENG-Q001–ENG-Q1167` без намеренных пропусков;
- `ENG-FR001–ENG-FR217`;
- Engineering workspace/change-set/object foundation;
- Types / Device Profiles / Object Templates и lifecycle/migrations;
- Connections / Adapters / Endpoints / Credentials / execution placement;
- complete Parameter/value/source/quality/freshness/history/substitution model;
- complete Semantic Command Model: identity, typed arguments, risk, preconditions/interlocks, rights, confirmations, approvals, technical binding;
- invocation lifecycle, evidence-based success, timeout/uncertainty, retries/idempotency, concurrency/cancellation/deferred execution;
- Full/Edge authority, offline execution, Rules/Scenarios/API/UI/service origins, diagnostics/simulation, bulk/scheduled/emergency/break-glass, versioning/impact;
- no-bypass invariant for any state-changing action of a modeled managed resource.

Checkpoint history:

- `ENG-CP01` — `688392edb17ddce6e4d3874ff54344aacc2033b0`;
- `ENG-CP02` — `fa38f437a90f98cdb4091a25187eec67f2213e6a`;
- `ENG-CP03` — `45756985f305ac0e952319d2b399262726beb964`;
- `ENG-CP04` добавляет `ENG-Q791–ENG-Q1167` и `ENG-FR151–ENG-FR217` и ожидает commit/push.

## 4. Следующая точка продолжения

После подтверждения SHA `ENG-CP04` продолжить с:

> `ENG-Q1168...` — Discovery proposal / Observed / Promotion / Import: source identity/incarnation, proposal review/matching, correction/rebind/split, promotion ownership, bulk discovery handling и strict import-to-draft semantics.

Далее закрыть full Validation / Impact / Approval / Publish / Deploy / Activate / Edge и остальные Engineering blocks по `ROADMAP.md`.

## 5. Checkpoint discipline

Пользователю не требуется вручную отслеживать момент фиксации. Правила находятся в `../functional/ROADMAP.md`:

- смысловой блок имеет приоритет над фиксированным количеством вопросов;
- ориентир — `100–200` новых Q между checkpoints;
- перед ZIP обязательны перенос всех Q/FR, coverage-check, roadmap и PROJECT_STATE update;
- диапазон следующего checkpoint заранее не фиксируется.

## 6. Architecture readiness

До `GATE-A` не выбирать по инерции frontend/backend frameworks, конкретную DB, message broker, internal transport, fencing/lease/consensus implementation или исходный код.

`GATE-A` выполняется после достаточной детализации Engineering + Operations + Web Platform и проверяет центральный сквозной contour от configuration до runtime/command/audit.
