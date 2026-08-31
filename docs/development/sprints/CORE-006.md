# CORE-006 — Device Manager

## Статус

**В разработке. Plan commit и Step 1 подтверждены; Step 2 фиксирует project/resource scope и authorization semantics. После подтверждения Step 2 следующий шаг — Step 3: durable metadata storage.**

Этап: `L1-01 — Ядро платформы`.

Проверенный baseline перед планированием:

`b8fed81d9f68e47e7635a283e1b2803166ca5bf8`

Это финальный documentation-closure commit завершённого `CORE-005 — Users & Access`.

Plan commit проверен в GitHub:

`e6259e9564d31df3bb677a1cced63e64c99ba534`

Step 1 commit проверен в GitHub:

`c79ff405284dbed1d0dc9ac3340f97dbcfa217cd`

## Цель

Создать отдельный backend-сервис Device Manager как источник истины для **описательной модели устройств и метрик**, не смешивая её с runtime-значениями Data Hub и не привязывая ядро к конкретному протоколу оборудования.

К завершению первого спринта Device Manager должен давать устойчивую основу, на которую смогут опереться Driver Runtime, Modbus/SNMP, Event Manager и будущий Web:

- stable opaque identity устройств и метрик;
- device/metric metadata;
- read/write свойства метрик;
- явную связь рабочей метрики с state-метрикой;
- durable локальное хранение metadata;
- versioned language-independent внешний contract;
- backend-authoritative access enforcement через существующие Users & Access / Service Hub boundaries;
- отдельный Linux lifecycle и real interprocess integration path;
- документацию для будущей Web-интеграции без разработки Web-кода в CORE-006.

## Исходные продуктовые решения

`docs/concept/05-devices-metrics.md` уже фиксирует:

- устройство — универсальный объект диспетчеризации, не обязательно один физический прибор;
- отдельная обязательная пользовательская классификация `physical/virtual` не вводится;
- метрика может относиться к устройству или существовать самостоятельно;
- метрика может быть read-only или writable;
- отдельной универсальной сущности `Command` в ядре нет: управление выполняется записью в writable metric;
- у каждой рабочей метрики имеется связанная state-метрика;
- Dashboard, Mimics, Event Manager и плагины работают с универсальными device/metric IDs, а не с конкретным протоколом.

Эти решения CORE-006 не переопределяет.

## Архитектурные границы

### Device Manager vs Data Hub

Device Manager владеет **описательной metadata**. Data Hub остаётся владельцем текущего runtime-value state.

Device Manager не должен хранить или кэшировать как authoritative state:

- текущее значение метрики;
- source timestamp текущего sample;
- realtime subscription state;
- факт принятия/непринятия конкретного runtime write request.

Data Hub уже считает `MetricId` opaque и сознательно не хранит описательную metadata. CORE-006 должен сохранить эту границу.

### Metric ID

Device Manager создаёт/хранит stable opaque metric IDs. Data Hub не должен извлекать из ID device, project, protocol, metric role или другие свойства.

Не кодируем предметную структуру в строку ID как обязательное правило протокола.

### Value type

Описательная модель должна уметь сообщить тип значения метрики, достаточный для согласования с текущим Data Hub `MetricValue` baseline (`bool`, signed/unsigned integer, floating point, string, bytes). Точное machine-readable представление фиксируется на Step 1/contract step и не должно создавать второй несовместимый type system.

### State metric

CORE-006 хранит связь рабочей метрики с её state-метрикой и обеспечивает referential integrity этой связи.

При этом CORE-006 **не**:

- утверждает окончательный enum `Normal/Warning/Alarm/NoData/Maintenance/...`;
- вычисляет состояние;
- выполняет Event Manager rules;
- публикует state value в Data Hub.

Точное domain-представление `working/state` и допустимые связи фиксируются Step 1 до persistence/schema.

### Projects и resource scope

Project является точкой консолидации/контекста, а не владельцем Device.

CORE-005 дал только global/project access scopes и намеренно не придумал Device-specific ACL до появления Device Manager. Поэтому CORE-006 обязан **до замораживания persistence и внешнего contract** определить минимальную модель связи device/metric с project context и policy доступа.

Ограничения этого решения:

- нельзя превращать Project entity в владельца Device;
- нельзя доверять project ID из client payload как доказательству доступа;
- нужно учитывать возможность повторного использования одного device/metric в нескольких контекстах, если это действительно требуется продуктовой модели;
- не создаём произвольный универсальный ABAC/resource-policy engine;
- Project Manager SQLite и Users & Access SQLite не становятся общей БД Device Manager;
- если project-resource association объективно не нужна для минимального корректного v1, её отсутствие должно быть явно обосновано и не должно приводить к раскрытию metadata пользователю без нужного доступа.

Это решение принимается Step 2 и затем становится частью contract/security semantics.

### Users & Access

Защищённые Device Manager operations используют существующий Service Hub transport `auth` и authoritative `users-access.v1/evaluate-access` либо другую уже согласованную Users & Access operation, если Step 2 докажет необходимость compatible extension.

Device Manager не получает user ID/roles/permissions из business payload и не читает Users & Access SQLite напрямую.

Временная недоступность security dependency не должна приводить к fail-open.

### Driver Runtime

CORE-008 будет определять реальную driver runtime boundary и ownership runtime sources/write providers.

Поэтому CORE-006 не выдумывает заранее:

- Modbus/SNMP-specific fields;
- driver process protocol;
- public registration protocol runtime providers;
- polling/connection/retry configuration конкретных протоколов;
- Data Hub write-provider межпроцессную boundary.

Device Manager должен лишь создать metadata foundation, которую CORE-008 сможет использовать без переделки базовой device/metric identity.

## Минимальная domain surface

Step 1 должен зафиксировать минимально достаточные сущности и invariants. Плановый baseline включает необходимость представить:

### Device

Как минимум:

- stable opaque `id`;
- human-readable `name`;
- optional/empty `description`;
- location/placement metadata в минимальной форме, если она действительно нужна текущей модели и Web handoff.

Не добавляем protocol/driver-specific configuration в базовый Device.

### Metric

Как минимум metadata должна покрыть:

- stable opaque `id`;
- optional association с Device, чтобы standalone metrics оставались допустимы;
- human-readable name/description;
- value type, совместимый по смыслу с Data Hub;
- unit/engineering unit metadata, если применимо;
- writable/read-only property;
- working/state relationship.

Step 1 обязан определить invariants state relationship: что считается state metric, может ли state metric иметь собственную state link, как запрещаются dangling/cyclic invalid relationships и какие свойства state metric нельзя трактовать как user-writable control.

Точный набор полей фиксируется Step 1 по минимальной необходимости и затем документируется в contract; этот план не является schema.

## Что сознательно не входит в первый CORE-006

- runtime current values и subscriptions — Data Hub;
- реальная запись в оборудование — Driver Runtime/драйверы;
- Modbus/SNMP configuration;
- Event Manager rules и state calculation;
- окончательный enum metric state;
- history/trends;
- Dashboard/Mimic configuration;
- Web Device Manager UI;
- generic command entity;
- arbitrary tags/query language без реального потребителя;
- delete/cascade lifecycle, если безопасная semantics удаления ещё не нужна следующему спринту;
- bulk import/export;
- production-final device templates.

Концепция предусматривает device templates, но их окончательная representation сознательно не проектируется в CORE-006 до появления реальных требований Package Manager/Driver Runtime. Если минимальный template primitive окажется необходим CORE-008, он добавляется отдельным обоснованным расширением, а не speculative API сейчас.

## План шагов

### Step 1 — Domain model и отдельный service skeleton

Создаём самостоятельный C++20 Device Manager service boundary и unit-test target.

Фиксируем:

- Device/Metric entities и stable opaque IDs;
- минимальные metadata fields;
- standalone metric semantics;
- value-type model, согласованную по смыслу с Data Hub;
- writable semantics;
- working/state metric representation и invariants;
- validation limits, достаточные для безопасного contract/persistence шага.

На этом шаге нет durable storage, Service Hub provider или Web.

**Результат:** domain/application core отдельно собирается и тестами доказывает базовые invariants без зависимости от Data Hub implementation.

#### Решение и реализация Step 1

Step 1 сознательно остаётся узким и не создаёт временный CRUD/storage/network API. Добавлен отдельный `services/device-manager/` C++20 service target и domain library без зависимости от Data Hub implementation.

Domain baseline:

- `Device`: stable opaque bounded `id`, `name`, `description`, `location`;
- `Metric`: stable opaque bounded `id`, optional `device_id`, `name`, `description`, semantic value type, `unit`, `writable`, `working/state` kind и optional state-link field;
- отсутствие `device_id` является нормальной representation standalone metric;
- value types семантически соответствуют текущему Data Hub `MetricValue`: `bool`, `int64`, `uint64`, `double`, `string`, `bytes`, но wire representation Device Manager пока не выбрана;
- каждая working metric обязана ссылаться на state metric;
- state metric read-only и не может иметь собственную state-link;
- catalog validation отклоняет неизвестный Device, dangling state target, ссылку на working metric вместо state metric и несовпадающую device association working/state пары;
- окончательный enum/encoding runtime state не выбран и остаётся вне Step 1.

Validation limits Step 1 выражены как UTF-8 byte-oriented storage/contract guardrails: opaque ID 256 bytes, name 256, description 4096, location 1024, unit 128. Они не являются решением о wire schema; Step 4 должен отразить фактическую domain semantics без второго несовместимого type system.

Standalone executable `dispatcher-device-manager` на Step 1 не принимает configuration arguments и только подтверждает независимую process boundary с clean `SIGINT`/`SIGTERM` lifecycle. CTest selection `^device-manager\.` содержит domain и два lifecycle tests.

На Step 1 намеренно отсутствуют:

- persistence/schema;
- project/resource association и access policy;
- Service Hub provider/contract;
- Users & Access integration;
- Data Hub calls/runtime values;
- Driver Runtime/protocol configuration;
- Web code.

### Step 2 — Project/resource scope и authorization semantics

До фиксации schema/contract определяем, как Device/Metric участвуют в project context и как применяются уже существующие `view`, `edit`, `admin`/`control` capabilities.

Нужно явно решить:

- где хранится project-resource association, если она нужна;
- может ли один resource использоваться в нескольких projects;
- какая policy применяется к global context;
- какие операции требуют `view`, `edit` или `admin`;
- относится ли `control` к metadata operations (по умолчанию нет: `control` предназначен для runtime write, а не редактирования metadata);
- как shared resource редактируется без cross-project privilege escalation;
- какой fail-closed результат ожидается при недоступной security dependency.

Не создаём Device-specific capability names, если существующих четырёх достаточно.

**Результат:** project/resource/access semantics определены до persistence и wire contract, Project не превращён в owner.

#### Решение Step 2

Step 2 является documentation-only архитектурным решением: persistence, wire operations и security client code ещё не создаются.

Project/resource model:

- project association хранится внутри Device Manager metadata, а не в Project Manager; Project остаётся контекстом, не владельцем ресурса;
- один Device может быть связан с несколькими projects; отсутствие associations допустимо и означает ресурс global catalog, а не «принадлежность глобальному проекту»;
- Metric, принадлежащая Device, наследует project associations Device и не может иметь отдельный независимый набор project associations;
- standalone Metric хранит собственный набор project associations; standalone working/state pair должна иметь одинаковый набор associations;
- association не меняет stable Device/Metric identity и не создаёт копий ресурса.

Authorization semantics для будущих protected metadata operations:

- project-context read/list требует effective `view` соответствующего project и возвращает только связанные с ним ресурсы;
- global catalog read/list требует global `view`; одни только project-scoped permissions не открывают global catalog;
- metadata mutation ресурса, связанного ровно с одним project, может быть разрешена effective project `edit` **или** project `admin`; capabilities остаются независимыми, поэтому проверяются явно;
- metadata mutation unassociated global-catalog ресурса или ресурса, shared между несколькими projects, требует global `edit` **или** global `admin`; это предотвращает cross-project privilege escalation через изменение общей metadata;
- изменение самих project associations требует global `admin`, поскольку оно меняет границу видимости/доступа между проектами;
- `control` не применяется к Device Manager metadata operations. Runtime write policy и control mode остаются отдельной будущей границей;
- при недоступности authoritative Users & Access любая authorization-dependent read/mutation должна fail closed: metadata не раскрывается и не изменяется.

Точные operation names, request payload и error codes остаются Step 4. Step 2 не расширяет `users-access.v1` и не вводит Device-specific capabilities.

### Step 3 — Durable metadata storage

Выбираем минимальную durable storage technology на основании фактической domain model Step 1–2. Если подходит SQLite, выбор фиксируется как service-local storage, а не общая БД платформы.

Реализуем:

- schema versioning;
- durable Device/Metric metadata;
- project/resource associations, если они приняты Step 2;
- referential integrity device/metric/state links;
- create/reopen/restart tests;
- predictable handling unsupported newer schema;
- storage errors без частично сохранённых invalid relationships.

**Результат:** metadata переживает restart и остаётся внутренней ответственностью Device Manager.

### Step 4 — Versioned external contract и Service Hub provider

Фиксируем language-independent внешний contract, предполагаемый service address `device-manager.v1`, и machine-readable schema.

Минимальная API surface должна покрывать реальные требования текущего этапа, ориентировочно:

- list/get devices;
- create/update device metadata;
- list/get metrics с понятным filter/context;
- create/update metric metadata;
- операции project/resource association только если они необходимы принятой Step 2 model.

Точные operation names/payload/errors определяются contract step, а не этим планом.

Contract не включает runtime current values или write execution: для них уже существует Data Hub.

Provider использует существующий Service Hub v1, отдельный executable остаётся independently runnable/reconnecting.

**Результат:** metadata доступна через стабильную process boundary без прямых C++ dependencies между сервисами.

### Step 5 — Users & Access enforcement

Подключаем production authorization через отдельную Service Hub client-role boundary, по уже проверенному паттерну Project Manager.

Проверяем:

- unauthenticated denial;
- authoritative identity только из transport session;
- list/get filtering/denial по принятой Step 2 scope policy;
- create/update authorization;
- access revocation на следующем authoritative request;
- disabled/expired session;
- Users & Access unavailable/invalid response => fail closed;
- отсутствие прямого доступа к Users & Access persistence.

Если Step 2 требует проверить существование project association через Project Manager, это подключается только через его внешний contract и только при реальной необходимости; прямой SQLite/C++ coupling запрещён.

**Результат:** Device Manager metadata не защищается только UI-фильтрацией и готова к будущему browser/service use.

### Step 6 — Real integration, lifecycle и restart recovery

Добавляем real interprocess acceptance для фактической topology CORE-006:

- Service Hub + Device Manager;
- Users & Access, когда операция защищена;
- Project Manager только если Step 2 сделал его реальной dependency;
- durable reopen/restart Device Manager;
- Service Hub restart/re-registration;
- Users & Access outage/recovery;
- allowed/denied/revoked paths;
- metadata persistence and state-link integrity через внешний contract;
- clean SIGINT/SIGTERM shutdown;
- отсутствие secret material в logs/test output.

Data Hub не запускается только ради церемонии, если Device Manager contract не делает runtime calls к нему. Вместо этого acceptance подтверждает, что Device Manager не дублирует current-value state и использует opaque metric IDs, совместимые с Data Hub boundary.

**Результат:** реальная service boundary работает независимо от process restart и security outage.

### Step 7 — Sprint acceptance, documentation audit и Web handoff

Новых product features не добавляем, кроме исправлений, необходимых acceptance.

Проводим:

- targeted C++ build/CTest Device Manager;
- real integration regression текущей topology;
- storage/restart/security/fail-closed checks;
- targeted audit `README`, architecture/contract, roadmap, sprint report, service README, context;
- обязательное заполнение Device Manager section в `docs/development/WEB_IMPLEMENTATION.md` по фактическому v1 contract: operations, metadata, scope, rights, states/errors, Data Hub runtime relation и открытые UX-вопросы.

Web-код не добавляется.

**Результат:** CORE-006 закрыт проверенным documentation-closure commit и даёт metadata foundation для `CORE-007 — Package Manager` и затем `CORE-008 — Driver Runtime`.

## Критерий завершения

CORE-006 завершён, когда одновременно выполнено:

1. Device Manager существует как отдельный C++20 executable/service с локальной сборкой и тестами.
2. Device/Metric identity и metadata invariants документированы и покрыты тестами.
3. Рабочая metric → state metric relation хранится с referential integrity без утверждения окончательного state enum.
4. Runtime values не дублируются в Device Manager и остаются Data Hub responsibility.
5. Durable metadata переживает restart; schema/version behavior предсказуемо.
6. Существует versioned language-independent external contract и real Service Hub provider.
7. Protected operations используют backend-authoritative Users & Access policy и fail closed при security outage.
8. Project/resource semantics определены явно и не превращают Project в owner.
9. Real interprocess acceptance проверяет allowed/denied/revoked/restart/re-registration paths, соответствующие фактической topology.
10. `docs/development/WEB_IMPLEMENTATION.md` содержит достаточный handoff будущему CORE-014 без чтения внутреннего Device Manager кода.
11. Targeted documentation audit завершён, итоговый report заполнен и closure SHA проверен в GitHub.

## Известные решения, которые сознательно оставлены будущим спринтам

- exact state enum — Event Manager/domain step, когда появится реальная state logic;
- driver binding/source configuration — CORE-008+;
- Modbus/SNMP fields — CORE-009/010;
- package/device-template delivery — CORE-007/008 по фактической необходимости;
- runtime write authorization + control-mode enforcement для physical write path — Driver Runtime/Data Hub integration, а не metadata CRUD;
- Web presentation — CORE-014;
- Dashboard/Mimic use — CORE-015/016.
