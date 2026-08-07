# Engineering & Configuration — Functional Specification

**ID namespace:** `ENG-*`  
**Статус:** `NOT STARTED` — каркас подготовлен для первого содержательного раунда.  
**Зависит от:** `PRD-Q001–PRD-Q803`.  
**Reference scenarios:** `RS-001`, `RS-002`, `RS-003`, `RS-004`, `RS-005`, `RS-012`, `RS-014`, `RS-019`.

## 1. Назначение

Спецификация описывает, как инженер создаёт, изменяет, проверяет, публикует и сопровождает конфигурацию Dispatcher от одного Compact-контроллера до распределённой Full/Edge установки.

Она должна дать однозначный ответ на практический вопрос:

> Инженер открыл Dispatcher и должен превратить пустую/существующую установку в корректно настроенную работающую систему. Какие рабочие области, сущности, действия, проверки и состояния он проходит?

## 2. Граница спецификации

Входит:

- Engineering workspace;
- locations и object registries;
- managed/observed objects;
- types, device profiles, templates;
- connections/adapters/endpoints;
- parameters и semantic commands;
- relationships;
- discovery;
- import;
- Create copies;
- draft/change sets;
- collaborative editing;
- validation;
- impact analysis;
- approval;
- publish;
- deploy/activate;
- desired/active state Edge;
- version/recovery views;
- engineering diagnostics;
- engineering permissions;
- simplified setup для Compact поверх той же модели.

Не входит как подробная предметная спецификация:

- ежедневная operator workflow — `OPS-*`;
- общие UI primitives/shell — `WEB-*`;
- внутренний protocol/API implementation;
- конкретная БД;
- реализация distributed configuration transport;
- детальный редактор Dashboard/Mimic/VMS/ACS/ТОиР, кроме точки интеграции с Engineering.

## 3. Неподвижные инварианты из Product Concept

Спецификация обязана сохранять:

- одна core identity объекта;
- type / device profile / object template разделены;
- configuration discovery/proposal ≠ authoritative runtime observation;
- import создаёт обычную новую конфигурацию в editor draft и не является sync/update механизмом;
- managed configuration проходит Draft → Validate → Impact → Approval(policy) → Publish;
- Publish ≠ Deploy ≠ Activate;
- Full задаёт desired operational Edge configuration, actual/active state сообщается отдельно;
- managed actuator action всегда проходит Command Model;
- package/service editor не получает параллельный Save/Apply lifecycle;
- normal configuration change не выполняется скрытой administrative transaction;
- Compact использует тот же foundation и может быть позднее включён как Edge без пересоздания проекта.

## 4. Предлагаемые разделы спецификации

### ENG-01 — Engineering entry point и workspace

Нужно определить:

- откуда инженер входит в Engineering;
- service navigation;
- выбор site/location/context;
- основные рабочие области;
- что является registry, что editor, что inspector;
- как сохраняется navigation context;
- как инженер видит текущий draft/change set.

### ENG-02 — Locations и object registry

Нужно определить:

- создание/редактирование location;
- physical placement и functional relations;
- heterogeneous object registry;
- стандартные columns/search/filter;
- object inspector;
- multi-select и bulk actions;
- managed vs observed representation;
- lifecycle/presence indicators.

### ENG-03 — Object creation model

Нужно определить пользовательский путь создания:

- простого managed object;
- functional object + physical unit;
- logical/calculated object;
- service/system object;
- observed object promotion;
- object from template/profile;
- object by copy.

### ENG-04 — Types, profiles, templates

Нужно определить отдельные UX и semantics для:

- equipment/object type;
- device profile;
- object template;
- template update/diff/selective update/detach;
- inherited/default values;
- package-provided vs organization-defined definitions.

### ENG-05 — Connections и adapters

Нужно определить:

- создание connection;
- выбор adapter/contribution;
- endpoint/network/serial settings;
- credentials/secret references;
- assigned executor;
- connection test как administrative transaction;
- connection diagnostics;
- shared connection и multiple devices;
- impact изменения connection.

### ENG-06 — Parameters

Нужно определить:

- создание/binding parameter;
- type, quantity, unit;
- source mapping;
- quality/freshness presentation;
- historization policy;
- alarm limits/policies integration;
- multiple sources/active source policy;
- substitution capability;
- derived/calculated parameter configuration;
- change compatibility analysis.

### ENG-07 — Semantic commands

Нужно определить:

- command definitions на type/profile/object уровне;
- parameters/arguments;
- risk class;
- preconditions/interlocks;
- success criterion;
- timeout/uncertain semantics;
- implementation binding к adapter;
- rights/policy impact;
- diagnostic low-level command separation.

### ENG-08 — Relationships

Нужно определить:

- создание typed relations;
- physical/functional/dependency/service relations;
- graph/list editing;
- validation cycles/incompatibilities;
- cross-service impact.

### ENG-09 — Discovery и observed objects

Нужно определить два разных UX:

1. discovery proposal → review → managed draft;
2. authoritative runtime observation → observed object без per-object publication.

Также:

- identity/source/incarnation status;
- promote to managed;
- rebind/split/merge correction;
- disappearance/presence lifecycle.

### ENG-10 — Import и Create copies

Нужно определить:

- file/paste entry;
- strict syntax feedback;
- whole-file syntax rejection;
- semantic errors loaded into draft;
- unknown columns;
- type conversion rules;
- preview Create copies;
- deterministic numbering/address rules;
- collisions as normal validation errors.

### ENG-11 — Draft и change sets

Нужно определить:

- active vs draft representation;
- named change set;
- autosave revisions/checkpoints;
- multiple editors in one change set;
- object-level concurrent editing;
- conflict representation/resolution;
- abandon/reopen/change ownership where allowed.

### ENG-12 — Validation

Нужно определить уровни:

- field/schema;
- object;
- relationship;
- service/domain;
- security/permissions;
- resource/performance;
- Full/Edge placement;
- package/dependency compatibility;
- command/safety checks;
- errors vs warnings;
- navigation from issue to source editor.

### ENG-13 — Impact analysis

Нужно определить:

- impacted objects/services/nodes;
- commands/alarms/automation dependencies;
- rights impact;
- package dependencies;
- Edge consistency domains;
- restart/deployment consequence;
- potentially irreversible effects;
- understandable vs technical detail.

### ENG-14 — Approval и Publish

Нужно определить:

- review summary;
- required approval by risk;
- self-approval restrictions where policy says;
- publication identity/version;
- atomicity within consistency domain;
- result states;
- failed publication handling;
- audit.

### ENG-15 — Deploy / Activate / Edge

Нужно определить:

- desired vs delivered/prepared/active;
- per-node status;
- activation waves where applicable;
- consistency-domain blocking;
- mixed-version allowed/blocked reasons;
- offline Edge;
- retry/reconciliation;
- authority handover visibility.

### ENG-16 — Versions, correction и recovery

Нужно определить:

- compare published versions;
- corrective publication instead of history rewrite;
- restore/recovery divergence;
- newer Edge active config after stale Full restore;
- explicit downgrade/recovery decision;
- history/provenance of configuration decisions.

### ENG-17 — Engineering diagnostics

Нужно определить:

- connection/adapter diagnostics;
- current executor;
- source values;
- quality/freshness;
- temporary tracing entry points;
- test command/diagnostic mode entry points;
- sanitized technical detail according to role.

### ENG-18 — Compact simple setup

Нужно определить упрощённый flow для малого объекта:

- ready composition profile;
- rooms/zones;
- hardware I/O binding;
- ready device/domain templates;
- simple rule/scenario setup;
- no loss of underlying engineering model;
- переход в professional Engineering без миграции формата.

## 5. Первый раунд вопросов

Содержательная проработка начинается с `ENG-Q001` и должна сначала определить **каркас Engineering UX**, а не детали конкретного протокола.

Рекомендуемый первый блок вопросов:

1. Какие верхнеуровневые рабочие области видит инженер?
2. Что открывается по умолчанию при входе в Engineering?
3. Как инженер выбирает location/context?
4. Registry и editor — это отдельные маршруты или один master-detail workflow?
5. Где живёт текущий change set и как он виден пользователю?
6. Как из object registry перейти к connection/type/profile/template без потери контекста?
7. Как отличается простой Compact setup от полного Engineering UX?

## 6. Критерий завершения

Спецификация готова к `REVIEW`, когда можно без архитектурных догадок пройти минимум:

- `RS-001` новый контроллер → live data;
- `RS-002` массовое копирование;
- `RS-003` discovery proposal;
- `RS-004` observed VM → promotion;
- `RS-005` Compact small-site setup;
- `RS-012` stale Full restore;
- `RS-014` combined object split;
- `RS-019` Edge authority handover.

## 7. Принятые functional requirements

Пока отсутствуют. Раздел заполняется после первых `ENG-Q...` решений.
