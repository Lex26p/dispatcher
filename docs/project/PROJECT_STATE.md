# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA перед этим пакетом:** `2ae985a8e99fb329e5860bd528007271966de3f4`.

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

Принято и подготовлено к `ENG-CP05`:

- `ENG-Q001–ENG-Q1527` без намеренных пропусков;
- `ENG-FR001–ENG-FR281`;
- Engineering workspace/change-set/object foundation;
- Types / Device Profiles / Object Templates и lifecycle/migrations;
- Connections / execution placement;
- complete Parameter/value/source/history/substitution model;
- complete Semantic Command Model;
- Discovery capabilities/candidates/matching/proposals;
- authoritative Observed Object source identity/incarnation/presence semantics;
- source-vs-managed ownership, Promotion/Manage и Commands on observed entities;
- governed Merge/Split/Rebind identity correction;
- Edge/multiple-source/continuous discovery behaviour;
- strict Import→Draft semantics: no update/delete/upsert/sync, typed schemas, strict parsing, collisions, dependencies, preview/provenance, large import and Compact wrapper.

Checkpoint history:

- `ENG-CP01` — `688392edb17ddce6e4d3874ff54344aacc2033b0`;
- `ENG-CP02` — `fa38f437a90f98cdb4091a25187eec67f2213e6a`;
- `ENG-CP03` — `45756985f305ac0e952319d2b399262726beb964`;
- `ENG-CP04` — `2ae985a8e99fb329e5860bd528007271966de3f4`;
- `ENG-CP05` добавляет `ENG-Q1168–ENG-Q1527`, `ENG-FR218–ENG-FR281` и ожидает commit/push.

## 4. Следующая точка продолжения

После подтверждения SHA `ENG-CP05` продолжить с:

> `ENG-Q1528...` — Configuration Governance Lifecycle: full Validation → Impact Analysis → Review/Approval → Publish.

После завершения этого lifecycle и `ENG-CP06` рекомендуется завершить текущий чат. Новый чат начать с Deploy / Activate / Edge и оставшихся Engineering completion blocks по `ROADMAP.md`.

## 5. Checkpoint discipline

Пользователю не требуется вручную отслеживать момент фиксации. Правила находятся в `../functional/ROADMAP.md`:

- смысловой блок имеет приоритет над фиксированным количеством вопросов;
- ориентир — `100–200` новых Q между checkpoints;
- перед ZIP обязательны перенос всех Q/FR, coverage-check, roadmap и PROJECT_STATE update;
- диапазон следующего checkpoint заранее не фиксируется.

## 6. Architecture readiness

До `GATE-A` не выбирать по инерции frontend/backend frameworks, конкретную DB, message broker, internal transport, fencing/lease/consensus implementation или исходный код.

`GATE-A` выполняется после достаточной детализации Engineering + Operations + Web Platform и проверяет центральный сквозной contour от configuration до runtime/command/audit.
