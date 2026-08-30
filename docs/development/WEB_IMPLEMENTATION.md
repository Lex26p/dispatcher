# Web implementation — отложенная интеграция и накопительный backlog

## Назначение

Этот документ хранит технический контекст, необходимый для будущего развития Web UI, пока основная разработка идёт backend-first.

Его задача — не заменить [`docs/concept/10-web-ui.md`](../concept/10-web-ui.md) и не дублировать внешние service contracts. Concept остаётся источником целевого UX, а документы в `docs/architecture/` — источником wire/API semantics. Здесь фиксируется **как будущий Web должен собрать уже реализованные backend-возможности в пользовательский интерфейс**, чтобы при возвращении к frontend не пришлось восстанавливать требования чтением всего backend-кода.

## Текущее решение о последовательности

Начиная после подтверждённого `CORE-005 / Step 7A` baseline, проект работает в режиме **backend first**.

- React + TypeScript остаются выбранным Web-стеком.
- Существующий committed Web Shell не удаляется и не переписывается.
- Незакоммиченная работа `CORE-005 / Step 7B` по Web control mode/security integration отменена и не является частью source of truth.
- Feature-разработка Web заморожена до завершения backend foundation `CORE-005`–`CORE-013`.
- В backend-спринтах Web-код не расширяется «заодно». Допустимы только исправления, необходимые для сохранения уже committed Web baseline при реальной несовместимости или критической ошибке.
- После `CORE-013` Web-разработка возобновляется отдельным sprint `CORE-014 — Web Integration & Core Operations UI`, после чего идут Dashboard и Mimics.

Причина такого порядка: backend contracts, права, lifecycle и service boundaries должны сначала стать устойчивыми. Это уменьшает стоимость постоянного переключения C++ ↔ React/Playwright и не заставляет frontend угадывать ещё меняющиеся backend semantics.

## Подтверждённый Web baseline до заморозки

Последняя функциональная Web-фаза до backend-first решения — `CORE-005 / Step 6B`, commit:

`ccde3a262d92ace53069d6e7740108b84f14aad9`

На этом baseline уже существуют:

- React + TypeScript Web Shell;
- один shared browser `ServiceHubClient` / WebSocket;
- public workspace и global navigation;
- browser session bearer только в `sessionStorage`;
- login/logout/current-user UX;
- authoritative `current-session` restoration;
- authenticated Project Manager requests;
- `/projects` и project context;
- `/access` для global admin;
- минимальный Users & Access administration UI.

`CORE-005 / Step 7A`, commit `f25aef1d3ff721f86487662289661409f72d3e57`, добавил **backend** control-mode contract и implementation, но Web control-mode UX на committed baseline отсутствует.

## Обязательное правило для backend-спринтов

Каждый backend-спринт, который создаёт или изменяет возможность, потенциально отображаемую/управляемую из Web, перед закрытием должен обновить этот документ.

Не нужно проектировать будущий экран до пикселя. Нужно оставить минимально достаточную интеграционную запись, чтобы Web-разработчику не пришлось искать semantics по исходникам сервиса.

Для каждой такой capability фиксируются:

1. **Service / version** — какой внешний контракт использовать.
2. **Пользовательская задача** — зачем функция нужна в Web.
3. **Операции и потоки** — request/response, subscription/realtime, write path.
4. **Scope/context** — global, project или другой реально введённый scope.
5. **Authorization** — какие capabilities проверяет backend и где Web может только отражать результат.
6. **Основные состояния** — loading, empty, active/inactive, unavailable, stale/reconnect и т. п., если они определены контрактом.
7. **Ошибки** — коды/классы ошибок, которые требуют осмысленного UX, а не generic failure.
8. **Запись/управление** — нужен ли control mode или дополнительный guard; Web никогда не заменяет backend authorization.
9. **Связи с project context** — должен ли текущий project фильтровать/задавать запросы.
10. **Неопределённые UX-вопросы** — что сознательно оставлено до Web-спринта.
11. **Backend baseline** — commit/спринт, на котором запись основана.

Если backend contract изменился, соответствующая запись обновляется в том же шаге. Этот файл не должен превращаться в копию schema: подробные payload остаются в `docs/architecture/*-contract.md`.

### Шаблон записи backend → Web

Для новых записей использовать компактную форму:

```text
### <Capability / surface>
Service/version: <service.vN или другой внешний contract>
Backend baseline: <CORE-xxx / commit после подтверждения>
User task: <что пользователь должен сделать/увидеть>
Operations/flow: <операции, subscription, write path>
Scope/context: <global/project/...>
Authorization: <view/control/edit/admin или другая реально введённая policy>
States: <важные contract states>
Errors/UX: <коды или классы ошибок, которые нужно различать>
Write/control: <нет / control mode / другой guard>
Project context: <как влияет текущий project>
Open UX questions: <что сознательно не решаем до Web sprint>
Sources: <contract docs>
```

В запись не нужно переносить внутренние C++ классы, SQLite-таблицы или implementation details, если Web их не использует. Если Web всё же зависит от конкретного lifecycle/ordering behavior, оно должно сначала быть выражено во внешнем contract или явно документированной service semantics.

## Накопленный Web backlog

### Service Hub

**Статус backend:** готов базовый browser-facing transport.

**Уже в Web:** shared connection, request/cancel path, connection status.

**Позже проверить:** reconnect UX и production transport/origin/TLS policy только после появления реальных deployment requirements.

**Источник:** `docs/architecture/service-hub-contract.md`, `CORE-002`, `CORE-003`, `CORE-005 / Step 4`.

### Project Manager

**Статус backend:** готов `project-manager.v1` с backend-authoritative Users & Access enforcement.

**Уже в Web:** list/create/edit, project context, authenticated requests.

**При возврате к Web:** пересмотреть project-context lifecycle уже поверх стабильного набора сервисов; не считать stored project snapshot доказательством доступа.

**Источник:** `docs/architecture/project-manager-contract.md`, `CORE-004`, `CORE-005 / Step 5`.

### Users & Access

**Статус backend:** `CORE-005` завершён. Identity, credentials, sessions, permissions, administration, durable security audit и control-mode backend baseline прошли финальный backend acceptance. Последний отдельно подтверждённый feature SHA до closure — `f25aef1d3ff721f86487662289661409f72d3e57`; Step 8 audit closure входит в финальный documentation-closure commit CORE-005.

**Уже в Web:** login/logout/current user, session restoration, minimal `/access` administration.

**Отложено в `CORE-014`:**

- control-mode presentation и enable/disable UX;
- единая явная модель lifecycle authenticated session + validated project context + control mode;
- real browser security acceptance поверх уже стабильных backend contracts;
- UX для access revocation/disabled/expired session и service recovery без превращения browser state в authority.

**Control mode contract:** project-scoped, требует backend `control`, fixed 10-minute absolute lifetime, не продлевается status reads, ephemeral и сбрасывается после Users & Access restart. Web должен показывать server state, но не считать mode отдельной permission grant.

Step 8 не добавляет новый Web API. Он только фиксирует durable backend audit явных `control_mode_enabled` / `control_mode_disabled`; UI control-mode semantics остаются теми же.

**Источник:** `docs/architecture/users-access-contract.md`, `CORE-005`.

### Data Hub

**Статус backend:** базовый realtime/current-value/write contract реализован в `CORE-001`.

**Будущий Web use:** отображение текущих значений/state и write-capable controls в Device/Dashboard/Mimic surfaces. Конкретные UI-компоненты не проектируются до появления Device Manager metadata и общей Web integration фазы.

**Источник:** `docs/architecture/data-hub-contract.md`, `CORE-001`.

### Device Manager — заполнить в CORE-006

Зафиксировать после реализации:

- device/metric metadata, которые реально нужны Web;
- read/write/state relationships;
- операции list/get/configure;
- project scope и permissions, если они реально вводятся;
- связь metadata с Data Hub runtime values;
- user-visible errors/validation.

### Package Manager — заполнить в CORE-007

Зафиксировать после реализации:

- package lifecycle operations;
- installation/enabled/disabled/error states;
- permissions;
- progress/restart requirements;
- какие package metadata должны отображаться.

### Driver Runtime / Modbus / SNMP — заполнить в CORE-008…CORE-010

Web не должен становиться driver-specific control plane без необходимости. Зафиксировать только реально нужные operator/admin diagnostics, configuration surfaces и error states, появившиеся в contracts этих спринтов.

### Event Hub — заполнить в CORE-011

Зафиксировать browser-facing event delivery/subscription semantics, ordering/reconnect expectations и ошибки, если они будут частью реального contract.

### Event Manager — заполнить в CORE-012

Зафиксировать user-visible event/alarm/state model, configuration operations, acknowledgement/handling semantics только в реально реализованном объёме и необходимые capabilities.

### System & Administration — заполнить в CORE-013

Зафиксировать system metrics/status/admin operations, permissions, service-state semantics и какие действия должны войти в Web administration surfaces.

## Возобновление Web после CORE-013

Перед первым кодовым шагом `CORE-014` нужно:

1. прочитать `docs/concept/10-web-ui.md`;
2. прочитать этот документ целиком;
3. прочитать architecture contracts сервисов, реально входящих в первый Web step;
4. сверить committed `web/README.md` и текущий frontend baseline;
5. составить шаги `CORE-014` до реализации;
6. только при обнаруженной неясности точечно читать backend implementation.

**Не начинать Web-фазу со сканирования всего `services/`.** Если документация backend-спринтов поддерживалась по этому правилу, необходимая semantics должна быть доступна из contracts + этого backlog.

## Предварительная граница CORE-014

`CORE-014 — Web Integration & Core Operations UI` должен собрать уже готовый backend foundation в цельный Web слой, не смешивая это с Dashboard constructor.

Ожидаемые темы, которые будут уточнены перед началом спринта:

- session/current-user/access UX на окончательном backend contract;
- project context;
- control mode;
- Device Manager surfaces;
- Package Manager surfaces;
- events/alarms operator surfaces;
- System & Administration surfaces;
- shared error/loading/reconnect patterns;
- real browser integration acceptance.

После этого отдельные спринты Dashboard и Mimics могут строиться поверх уже устойчивого Web/service integration layer.

## Что этот документ не решает заранее

До Web-фазы не выбираются без реальной необходимости:

- новый state-management framework;
- router library;
- component/UI library;
- design system implementation details;
- точная структура всех routes;
- production browser deployment topology;
- frontend caching strategy;
- универсальная форма всех редакторов.

React + TypeScript остаются текущим baseline. Любая будущая смена frontend stack требует отдельного решения и причины; текущая backend-first пауза сама по себе такой причиной не является.
