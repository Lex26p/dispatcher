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
- exact control-mode representation/expiration policy до `CORE-005 / Step 7`;
- production TLS/origin policy Service Hub;
- frontend state manager и UI-библиотеки.

## Текущая точка разработки

Завершены:

- `CORE-001 — Data Hub`;
- `CORE-002 — Service Hub`;
- `CORE-003 — Web Shell`;
- `CORE-004 — Project Manager`.

Финальный documentation baseline CORE-003:

`88c5bb30f182f7d9898ad4c95b210a045060c94f`

Sprint plan CORE-004:

`d36aaa5cdcbdfc0a2d95490d08fd46ab01c1db41`

Функциональные commits CORE-004:

- Step 1 — `172e40887fde3b5b963264904e0c4fa73225a34a`;
- Step 2 — `1d90091ee7804d1bb8c49618a1780a226488c671`;
- Step 3 — `7eb7c89e3f5d9b975dc10b71f4a1bbff8e00ed29`;
- Step 4 — `7527e2758f77e40cb6b86795e2b2a21896e55224`;
- Step 5 — `1b4017526789e32e5e4ece90a63fa287cddb57c8`;
- Step 6 — `9818254650fdf26ba8a2708dacca16433989d8fe`.

Closure CORE-004:

`29b1f0ea750633cc53cc4e023585835d2b06ad8b`

CORE-004 даёт отдельный C++ Project Manager, local durable SQLite storage, versioned `project-manager.v1` Service Hub provider, Web list/editor, shared frontend project context и реальный browser/backend restart-recovery acceptance path. Проект не владеет будущими Dashboard/Device/resources; auth/ACL не имитировались.

Текущий спринт — `CORE-005 — Users & Access`.

План: `docs/development/sprints/CORE-005.md`.

Plan commit проверен:

`d05cba25981599baaeadd9ad452d1f68dbabd834`.

CORE-005 должен дать stable user identity, durable users/access configuration, local authentication/session boundary, согласованный authenticated Service Hub request path, backend-authoritative Project Manager access, Web login/current-user/access administration и control-mode baseline. Первый sprint реально применяет global + project scope; Device/Dashboard-specific ACL, external IdP/MFA и Event Hub audit publication не имитируются.

Функциональные commits CORE-005 на текущий момент:

- Step 1 — `e8b42e69fecf7079f7b18f5f86fe334308d2579c`;
- Step 2 — `3b140d808638bb0c14a70b2aa1df96eb377af197`;
- Step 3 — `02a2d86e730a6c73ee0c33250bb9d4dc14791681`;
- Step 4 — `eb5f876e4a35dcbc5b5597e456a1197cc0d9dd1b`;
- Step 5 — `ebe98d1f16e55d4024438300c1670ec3a19b1d72`;
- Step 6A — `04e83879c73e298d1eac61acbd8e861f0ba5988d`.

Documentation sync после Step 5: `ef0b94ce88af77a7032b7e34f1d7a141cf16cd60`. Step 6A functional commit `04e83879c73e298d1eac61acbd8e861f0ba5988d` является baseline текущего Step 6B.

Step 1 зафиксировал stable user ID, `login`/`display_name`/`enabled`, independent capabilities `view/control/edit/admin`, named permission sets, global/project assignments и effective permissions как union matching assignments. Disabled user fail-closed; explicit deny/groups/ABAC не добавлены.

Step 2 зафиксировал локальный SQLite schema v1 только для Users & Access и OpenSSL scrypt password verifier (`N=2^17`, `r=8`, `p=1`) без plaintext storage. Explicit first-admin bootstrap читает password/confirmation через stdin и атомарно создаёт user + full explicit permission set + global assignment + credential verifier + audit record.

Step 3 зафиксировал `users-access.v1`, opaque 256-bit server-side session, 30-minute idle / 12-hour absolute lifetime, SQLite schema v2 с token digest и language-independent auth/session contract.

Step 4 совместимо добавил optional Service Hub v1 transport `auth: {type: "session", token}` отдельно от business payload, per-request auth option в browser `ServiceHubClient` и production `users-access.v1` provider с authoritative validation `logout`/`current-session`/`evaluate-access`. `login` остаётся public. Project Manager policy на Step 4 не менялась.

Step 5 защитил реальный Project Manager через Users & Access: unauthenticated deny, backend filtering `list-projects`, project-scoped `view`/`edit`/`admin`, global `admin` create, disabled/expired/revoked behavior, fail-closed при Users & Access outage и reconnect regression. Project v1 business payload не получил `user_id`, roles, permissions или auth token.

Step 6A завершён commit `04e83879c73e298d1eac61acbd8e861f0ba5988d`: production `users-access.v1` реализует зарезервированные administration operations с global `admin`, atomic user+credential creation и real Service Hub tests.

**Текущий локальный подшаг — `CORE-005 / Step 6B — Web authenticated context и administration UI`.**

Step 6B фиксирует browser session policy: opaque bearer хранится только в текущем browser `sessionStorage` (`dispatcher.user-session.v1`), reload authoritative восстанавливается через `current-session`, invalid/expired session очищается, а protected Web requests получают bearer через shared session-aware Service Hub wrapper без второго WebSocket. Web получает login/logout/current user, authenticated Project Manager path и global-admin `/access`; project context очищается при logout/user change и authoritative access revocation.

После проверенного Step 6B SHA выполняется `Step 6C — administration security audit completion`, чтобы закрыть уже обязательный audit значимых admin mutations до Step 7. Control mode остаётся Step 7.

## Рабочий процесс

Планирование имеет три уровня: этап → спринт → шаг. Этапы предварительно задают направление. Спринты конкретного этапа агент расписывает перед началом этого этапа; спринты текущего `L1-01` уже зафиксированы. Перед началом каждого нового спринта агент обязан сначала расписать локальные шаги в sprint-файле, чтобы реализация двигалась по заранее определённой последовательности и не раздувала scope.

Roadmap можно корректировать, но это исключение. Локальную техническую необходимость сначала следует решать изменением/разделением шагов текущего спринта; изменение этапов или уже согласованных спринтов требует объективной причины и явной фиксации.

Изменения ChatGPT отдаёт архивом с готовыми файлами проекта. Пользователь распаковывает его поверх `C:\Projects\dispatcher`, выполняет необходимые проверки, коммитит и отправляет SHA. Новый SHA становится базовой точкой истины.

Web frontend проверяется нативно в Windows/PowerShell через `npm.cmd` / `npx.cmd` на зафиксированном Node.js/npm toolchain. C++ backend собирается и тестируется в Linux/WSL.

Документация обновляется по ходу каждого шага, если шаг изменил уже документированный факт, контракт, технологическое решение, статус, команды или важное ограничение. Не следует откладывать такие решения до конца спринта и затем восстанавливать их по чату.

В конце каждого спринта дополнительно обязательна целевая проверка актуальности связанной документации. Финальный audit является проверкой целостности и пропущенной рассинхронизации, а не первой попыткой описать уже завершённый спринт.

В пользовательском Git-workflow не используются команды `git diff`.

Репозиторий: `https://github.com/Lex26p/dispatcher`.
