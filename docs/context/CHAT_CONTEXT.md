# Контекст проекта для продолжения в новом чате

## Назначение

Этот файл быстро восстанавливает контекст проекта «Диспетчер».

Репозиторий `Lex26p/dispatcher` является источником истины. Перед разработкой обязательно прочитать корневой `AGENTS.md`. Для продуктового/архитектурного контекста сначала следует читать `docs/README.md`, затем нужные concept-файлы, `docs/architecture/README.md`, roadmap и файл текущего спринта.

## Продукт

«Диспетчер» — универсальная модульная платформа диспетчеризации и управления физическими и цифровыми объектами.

Ядро небольшое и универсальное. Необязательные возможности выносятся в пакеты. Если пакет не установлен, его функций не должно быть в интерфейсе.

Система предназначена и для инженеров, и для программистов: типовые задачи предпочтительно решаются через UI, сложные — могут решаться кодом.

Dashboard — основной универсальный механизм построения Web UI.

## Согласованное ядро

- проекты;
- Dashboard;
- мнемосхемы;
- устройства;
- метрики, включая записываемые и state-метрики;
- диспетчер событий;
- пользователи и права;
- системные данные и базовое администрирование;
- менеджер пакетов;
- общий Web UI.

Автоматизация не входит в ядро и является первым планируемым официальным плагином.

## Ключевые продуктовые решения

### Проекты
Проект — точка консолидации, а не владелец данных. Проекты не вложены друг в друга. Пользователь с доступом к нескольким проектам может видеть объединённые события.

### Dashboard
Универсальная страница из контейнеров и компонентов. Может получать параметры от проекта и передавать их вложенным компонентам. Отвечает за страницы, контейнеры, модальные окна и навигацию.

### Мнемосхемы
Интерактивные SVG-изображения. Отображаются только внутри Dashboard. Не являются отдельными Web-страницами и не содержат другие мнемосхемы внутри себя.

### Устройства и метрики
Устройство может быть виртуальным, но это не отдельная пользовательская классификация.

Метрика — универсальная единица данных.

Отдельная универсальная сущность `Command` удалена. Управление выполняется записью в метрики, для которых разрешена запись.

У каждой рабочей метрики есть связанная state-метрика с одним текущим состоянием. Примеры: Normal, Warning, Alarm, NoData, Maintenance. Точный enum ещё не утверждён.

### Пользователи
Различаются просмотр, управление, редактирование и административные права. Опциональный режим управления защищает от случайной записи. Пользователь без права управления не может включить этот режим.

### Web UI
Компактный глобальный Header, максимум места рабочей области, левая навигация сервиса, правая панель свойств, локальный Header только при необходимости, редко используемые ссылки скрываются из постоянного интерфейса.

## Архитектура

Backend строится как набор независимых сервисов вокруг специализированных Hub-механизмов.

### Data Hub
- реализован в `CORE-001` как отдельный C++20-сервис;
- внешний контракт: gRPC + Protocol Buffers proto3, package `dispatcher.data_hub.v1`;
- принимает realtime-значения через `PublishMetric`;
- хранит последнее актуальное значение каждой метрики;
- отдаёт его через `GetCurrent`;
- поддерживает retained/live `Subscribe`;
- хранит state-метрики тем же механизмом, что и обычные метрики;
- принимает `WriteMetric` и маршрутизирует запрос к текущему внутреннему provider;
- не считает write request подтверждением фактического изменения значения;
- не знает предметную область и описательную metadata метрик.

### Device Manager
Источник истины об устройствах и описании метрик: ID, название, описание, местоположение, состав метрик, настройки и возможность записи.

Realtime-значения находятся в Data Hub.

### Диспетчер событий / аварий
Следит за настроенными метриками, проверяет условия и записывает нужный state обратно в Data Hub.

События и аварии отправляются в отдельный Event Hub.

### Event Hub
Отдельный Hub для событий и аварий. Не смешивается с Data Hub.

### Service Hub
- реализован и завершён как базовый сервис в `CORE-002`;
- внешний transport v1: WebSocket;
- serialization: UTF-8 JSON;
- endpoint path: `/v1/ws`;
- subprotocol: `dispatcher.service-hub.v1`;
- provider регистрирует один service address на connection;
- client отправляет addressable request по `service` + `operation`;
- Hub выполняет correlation, timeout/cancel и возвращает response;
- generic payload остаётся непрозрачным JSON;
- тот же client protocol напрямую пригоден browser Web Shell;
- `CORE-005 / Step 4` добавил optional transport `auth: {type: "session", token}` отдельно от business payload;
- Hub проверяет только форму session auth, не декодирует token и не создаёт trusted user/permissions;
- authoritative session/access validation принадлежит Users & Access и защищённым providers.

Подробный контракт: `docs/architecture/service-hub-contract.md`.

### Project Manager
- реализован и завершён в `CORE-004` как отдельный C++20-сервис;
- Project v1 содержит только stable opaque `id`, `name`, `description`;
- production persistence — локальный SQLite schema v1, внутренний для Project Manager;
- внешний service address — `project-manager.v1` поверх существующего Service Hub v1;
- операции — `create-project`, `list-projects`, `get-project`, `update-project`;
- Service Hub envelope не содержит project-specific field;
- Web Shell имеет `/projects`, list/create/edit UI и общий project context с явным global mode;
- project context сохраняется только в browser `sessionStorage`, проверяется через `get-project` и не является доказательством доступа;
- реальная browser integration подтверждает durable restart/re-registration path;
- `CORE-005 / Step 5` добавил server-side authorization поверх той же Project Manager boundary без `user_id`/permissions в Project v1 payload;
- все Project Manager v1 operations требуют session auth и используют authoritative `users-access.v1/evaluate-access`;
- create требует global `admin`, list/get используют `view`, update — `edit` либо `admin`;
- Users & Access outage/invalid response приводит к `project.authorization_unavailable`, business operation не выполняется.

Подробный контракт: `docs/architecture/project-manager-contract.md`.

### Users & Access
- `CORE-005 / Step 1` создал отдельную domain/application boundary со stable user identity, permission sets и global/project assignments;
- capabilities `view`, `control`, `edit`, `admin` независимы и не имеют скрытой иерархии;
- `CORE-005 / Step 2` выбрал локальный SQLite storage только для Users & Access и OpenSSL scrypt credential verifier без plaintext;
- secure first-admin bootstrap выполняется явно и читает password через stdin;
- `CORE-005 / Step 3` зафиксировал `users-access.v1`, opaque server-side session, 30-minute idle / 12-hour absolute lifetime и SQLite schema v2 с SHA-256 token digest;
- `CORE-005 / Step 4` подключил production `users-access.v1` provider через существующий Service Hub;
- public `login`; protected session-core operations `logout`, `current-session`, `evaluate-access`;
- protected operation получает bearer только из Service Hub transport auth context, а не из business payload;
- `CORE-005 / Step 5` подтвердил Users & Access как authoritative authorization dependency реального Project Manager;
- `CORE-005 / Steps 6A–6C` реализовали administration API и atomic durable audit user/permission/assignment mutations;
- `CORE-005 / Step 7A` реализовал project-scoped ephemeral control mode: effective `control`, 10-minute absolute lifetime, access-revocation fail-closed и reset после Users & Access restart;
- `CORE-005 / Step 8` добавил durable audit explicit `control_mode_enabled` / `control_mode_disabled` и подтвердил backend acceptance;
- local security audit не содержит password/raw bearer token.

Подробный контракт: `docs/architecture/users-access-contract.md`.

### Независимость
Сервис не должен знать других потребителей своих данных без необходимости. Плагины работают через общие контракты.

## Языки

Backend: C++20.

Текущий backend toolchain baseline: CMake 3.20+, Ninja, CTest, Linux/WSL.

Можно использовать готовые C++ библиотеки и создавать собственные по реальной необходимости.

Web: React + TypeScript.

Node.js — frontend toolchain, а не обязательный backend-сервис.

Межсервисные контракты должны быть независимы от C++, чтобы внешние плагины могли при необходимости использовать другие языки.

## Пока не выбрано

Для Data Hub и Service Hub transport/serialization уже выбраны. Project Manager использует локальный SQLite schema v1; Users & Access — локальный SQLite schema v2. Это не определяет общую persistence-технологию платформы.

Пока не утверждены:

- технология Event Hub;
- общая persistence-стратегия будущих сервисов и истории;
- deployment;
- внешний Driver Runtime API и межпроцессный путь write-provider;
- механизм восстановления runtime state Data Hub;
- точный state enum;
- production TLS/origin policy Service Hub;
- frontend state manager и UI-библиотеки.

## Текущая точка разработки

Завершены:

- `CORE-001 — Data Hub`;
- `CORE-002 — Service Hub`;
- `CORE-003 — Web Shell`;
- `CORE-004 — Project Manager`;
- `CORE-005 — Users & Access`.

Closure CORE-004:

`29b1f0ea750633cc53cc4e023585835d2b06ad8b`

Plan CORE-005:

`d05cba25981599baaeadd9ad452d1f68dbabd834`

Подтверждённые commits CORE-005 до финального closure:

- Step 1 — `e8b42e69fecf7079f7b18f5f86fe334308d2579c`;
- Step 2 — `3b140d808638bb0c14a70b2aa1df96eb377af197`;
- Step 3 — `02a2d86e730a6c73ee0c33250bb9d4dc14791681`;
- Step 4 — `eb5f876e4a35dcbc5b5597e456a1197cc0d9dd1b`;
- Step 5 functional — `ebe98d1f16e55d4024438300c1670ec3a19b1d72`;
- Step 5 documentation sync — `ef0b94ce88af77a7032b7e34f1d7a141cf16cd60`;
- Step 6A — `04e83879c73e298d1eac61acbd8e861f0ba5988d`;
- Step 6B — `ccde3a262d92ace53069d6e7740108b84f14aad9`;
- Step 6C — `382e4be446dbc3a4cf8b76cc4a88a67eaff6ba59`;
- Step 7A — `f25aef1d3ff721f86487662289661409f72d3e57`;
- backend-first staging — `bd52d4a5b651bef6685ed2b3a3292c3af841182b`;
- final CORE-005 closure — `b8fed81d9f68e47e7635a283e1b2803166ca5bf8`.

CORE-005 даёт stable user identity; independent `view/control/edit/admin`; global/project assignments; local SQLite schema v2; OpenSSL scrypt credentials; explicit first-admin bootstrap; opaque durable server-side sessions; authenticated Service Hub request boundary; production `users-access.v1`; backend-authoritative Project Manager authorization; administration API + atomic security audit; project-scoped ephemeral control mode.

Step 8 закрыл audit explicit control-mode enable/disable без schema/wire migration и прошёл backend acceptance: полный `^users-access\.` CTest selection и `project-manager.service-hub-integration`, включая fail-closed outage/recovery, revoked/disabled/expired paths, restart/re-registration и secret-leakage regression.

Final CORE-005 baseline проверен в GitHub: `b8fed81d9f68e47e7635a283e1b2803166ca5bf8` (`Complete CORE-005 Users and Access`).

Незакоммиченный Step 7B Web control-mode/security integration не является source of truth. Web feature-разработка заморожена до завершения backend foundation через `CORE-013`; накопительный handoff ведётся в `docs/development/WEB_IMPLEMENTATION.md`.

**Текущий спринт — `CORE-006 — Device Manager`.** План зафиксирован в `docs/development/sprints/CORE-006.md`. После подтверждения plan commit текущий шаг — Step 1: domain model и отдельный C++20 service skeleton. CORE-006 хранит device/metric metadata, а Data Hub остаётся источником runtime values. Project/resource access model должна быть определена до persistence/contract без превращения Project в owner. React + TypeScript не пересматриваются; Web в CORE-006 не развивается, а будущий handoff обновляется в `WEB_IMPLEMENTATION.md`.

## Рабочий процесс

Планирование имеет три уровня: этап → спринт → шаг. Этапы предварительно задают направление. Спринты конкретного этапа агент расписывает перед началом этого этапа; спринты текущего `L1-01` уже зафиксированы. Перед началом каждого нового спринта агент обязан сначала расписать локальные шаги в sprint-файле, чтобы реализация двигалась по заранее определённой последовательности и не раздувала scope.

Roadmap можно корректировать, но это исключение. Локальную техническую необходимость сначала следует решать изменением/разделением шагов текущего спринта; изменение этапов или уже согласованных спринтов требует объективной причины и явной фиксации.

Изменения ChatGPT отдаёт архивом с готовыми файлами проекта. Пользователь распаковывает его поверх `C:\Projects\dispatcher`, выполняет необходимые проверки, коммитит и отправляет SHA. Новый SHA становится базовой точкой истины.

Web frontend при Web-работе проверяется нативно в Windows/PowerShell через `npm.cmd` / `npx.cmd` на зафиксированном Node.js/npm toolchain. В текущей backend-first фазе backend-only шаги не обязаны запускать Web tests, если не меняют committed browser-facing contract. C++ backend собирается и тестируется в Linux/WSL.

Документация обновляется по ходу каждого шага, если шаг изменил уже документированный факт, контракт, технологическое решение, статус, команды или важное ограничение. Не следует откладывать такие решения до конца спринта и затем восстанавливать их по чату. Во время backend-first фазы новые backend capabilities, которые позже потребуются Web, дополнительно фиксируются в `docs/development/WEB_IMPLEMENTATION.md`, чтобы будущая frontend-фаза опиралась на contracts/docs, а не на поиск semantics по исходникам.

В конце каждого спринта дополнительно обязательна целевая проверка актуальности связанной документации. Финальный audit является проверкой целостности и пропущенной рассинхронизации, а не первой попыткой описать уже завершённый спринт.

В пользовательском Git-workflow не используются команды `git diff`.

Репозиторий: `https://github.com/Lex26p/dispatcher`.
