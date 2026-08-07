# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA перед этим пакетом:** `e47b3a2003bda70385903aaa26d126d7089542b3`.

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

Принято и подготовлено к `ENG-CP06`:

- `ENG-Q001–ENG-Q1953` без намеренных пропусков;
- `ENG-FR001–ENG-FR357`;
- Engineering workspace/change-set/object foundation;
- Types / Device Profiles / Object Templates и lifecycle/migrations;
- Connections / execution placement;
- complete Parameter/value/source/history/substitution model;
- complete Semantic Command Model;
- Discovery / authoritative Observed / Promotion / governed identity correction;
- strict Import→Draft semantics;
- complete Configuration Governance Lifecycle: revision-bound Validation, Impact, Review/Approval policies, Publish consistency/atomicity/failures/corrective lineage.

Checkpoint history:

- `ENG-CP01` — `688392edb17ddce6e4d3874ff54344aacc2033b0`;
- `ENG-CP02` — `fa38f437a90f98cdb4091a25187eec67f2213e6a`;
- `ENG-CP03` — `45756985f305ac0e952319d2b399262726beb964`;
- `ENG-CP04` — `2ae985a8e99fb329e5860bd528007271966de3f4`;
- `ENG-CP05` — `e47b3a2003bda70385903aaa26d126d7089542b3`;
- `ENG-CP06` добавляет `ENG-Q1528–ENG-Q1953`, `ENG-FR282–ENG-FR357` и ожидает commit/push.

## 4. Следующая точка продолжения

После подтверждения SHA `ENG-CP06` **завершить текущий чат**. Новый чат начать с:

> `ENG-Q1954...` — Deploy / Activate / Edge: Published Desired → delivery → prepare/readiness → activation, offline/partial Edge, per-node actual state, retry/reconciliation и authority-safe activation.

После этого завершить Versions / Recovery / Engineering diagnostics / Permissions / Compact setup и выполнить полный Engineering coverage review по `ROADMAP.md`.

## 5. Checkpoint discipline

Пользователю не требуется вручную отслеживать момент фиксации. Правила находятся в `../functional/ROADMAP.md`:

- смысловой блок имеет приоритет над фиксированным количеством вопросов;
- ориентир — `100–200` новых Q между checkpoints;
- перед ZIP обязательны перенос всех Q/FR, coverage-check, roadmap и PROJECT_STATE update;
- диапазон следующего checkpoint заранее не фиксируется.

## 6. Architecture readiness

До `GATE-A` не выбирать по инерции frontend/backend frameworks, конкретную DB, message broker, internal transport, fencing/lease/consensus implementation или исходный код.

`GATE-A` выполняется после достаточной детализации Engineering + Operations + Web Platform и проверяет центральный сквозной contour от configuration до runtime/command/audit.
