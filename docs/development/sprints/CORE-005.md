# CORE-005 — Users & Access

## Статус

**В разработке. Steps 1–5 и Step 6A/6B завершены; текущий подшаг — Step 6C.**

Этап: `L1-01 — Ядро платформы`.

Проверенный baseline перед планированием:

`29b1f0ea750633cc53cc4e023585835d2b06ad8b`

Это documentation-closure commit завершённого `CORE-004 — Project Manager`.

Plan commit зафиксирован и проверен в репозитории:

`d05cba25981599baaeadd9ad452d1f68dbabd834`

Текущий подшаг — `Step 6C — administration security audit completion`.

## Цель

Добавить в ядро реальный пользовательский контекст, аутентификацию и контроль доступа так, чтобы backend, Service Hub и Web перестали доверять неаутентифицированному пользовательскому запросу там, где требуется защищённое действие.

К завершению спринта пользователь должен иметь возможность:

- войти в Web Shell под локальной учётной записью;
- выйти из пользовательской сессии;
- видеть текущего пользователя;
- иметь разные права в разных проектах;
- получать только доступные ему проекты;
- выполнять разрешённые Project Manager операции и получать предсказуемый отказ при отсутствии прав;
- администрировать базовые users/access assignments при наличии административного права;
- использовать режим управления только при наличии права управления;
- сохранять понятное поведение Web Shell при logout, истечении/недействительности сессии и временной недоступности backend.

`CORE-005` должен создать reusable security boundary для будущих Device Manager, Package Manager, Dashboard и других сервисов, но не должен заранее моделировать их предметные ACL.

## Продуктовые исходные решения

Концепция `docs/concept/07-users-access.md` уже фиксирует:

- проект является одной из основных областей разграничения доступа, но не единственной возможной областью;
- система не должна быть жёстко привязана к фиксированным ролям `оператор/инженер/администратор`;
- должны поддерживаться роли/группы прав либо собственные наборы прав;
- концептуально различаются просмотр, управление, редактирование/настройка и административные действия;
- один пользователь может иметь разные права в разных проектах;
- доступ должен действовать последовательно во всём приложении, а не только скрывать элементы UI;
- режим управления является дополнительной защитой от случайного управления и доступен только пользователю с правом управления;
- значимые пользовательские действия должны фиксироваться;
- Web должен по возможности объяснять причину недоступного действия.

`CORE-005` не должен переопределять эти решения.

## Исходные архитектурные границы

### Service Hub

`CORE-002` намеренно оставил authentication/authorization за пределами Service Hub v1 baseline.

Текущий request envelope не содержит user ID, roles, permissions, project access, control mode или auth token. Эти поля нельзя добавлять как случайные provider-specific payload fragments.

`CORE-005` должен определить согласованный browser-compatible способ передать authenticated user/session context через уже существующую Service Hub boundary.

Требования:

- не вводить второй transport только ради authentication;
- не доверять user ID/permissions, присланным внутри предметного service payload;
- не превращать Service Hub в владельца Project Manager permission semantics;
- сохранить существующие request/correlation/cancel semantics;
- не ломать backend provider/client separation без необходимости;
- при необходимости compatible change текущего v1 должен быть явно зафиксирован contract/schema tests; если compatibility невозможна, versioning выбирается явно, а не скрыто.

Точная representation authentication/session context в Service Hub **не выбирается в этом плане заранее**. Она фиксируется на Step 4 после появления реального Users & Access session contract.

### Project Manager

`CORE-004` специально не добавлял user/role/permissions/auth/project ACL в Project Manager payload.

После `CORE-005` Project Manager должен использовать доверенную security boundary, а не принимать идентичность пользователя из `create-project`/`list-projects`/`get-project`/`update-project` payload.

Project context в Web Shell остаётся навигационным/frontend context и не становится доказательством доступа.

### Future services

Device Manager, Dashboard, Package Manager и другие будущие сервисы ещё не реализованы.

Поэтому `CORE-005` создаёт reusable access model и authorization boundary, но реально интегрирует её только с уже существующим Project Manager и Web Shell.

Не создаём фиктивные Device/Dashboard/resource ACL records до появления соответствующих contracts.

## Минимальная access model

На Step 1 фиксируется минимальная модель, достаточная для реальной авторизации существующей платформы.

Базовые требования:

- у пользователя есть stable opaque ID, независимый от login/display properties;
- login/credential identity отделена от mutable display properties;
- пользователь может быть enabled/disabled;
- права не кодируются жёстко единственной фиксированной ролью;
- существует assignable permission set/access profile как именованный набор capabilities;
- минимум capabilities покрывает концептуальные `view`, `control`, `edit`, `admin`;
- assignment имеет явный scope;
- в первом спринте реально поддерживаются global scope и project scope;
- один пользователь может иметь разные effective permissions в разных проектах;
- access evaluation является server-side authoritative;
- Web presentation не является security boundary.

Точные machine-readable names, merge semantics нескольких assignments и минимальный набор admin operations фиксируются Step 1 и contract step, а не угадываются в плане.

В первом sprint автоматически не добавляются:

- explicit deny rules;
- nested groups/roles;
- organizational hierarchy/tenants;
- arbitrary ABAC expressions;
- Device/Dashboard-specific scopes;
- per-field permissions;
- resource ownership model.

Если один из этих механизмов окажется реально необходим для критерия завершения, изменение плана должно быть зафиксировано отдельно.

## Authentication boundary

Ядро должно быть пригодно к локальному самостоятельному запуску без обязательного внешнего identity provider.

Поэтому `CORE-005` должен дать безопасный локальный authentication baseline и secure bootstrap первого администратора.

При этом план заранее не фиксирует:

- конкретный password hashing algorithm/library/parameters;
- opaque или self-contained session/token representation;
- session lifetime/rotation details;
- persistent или restart-invalidated sessions;
- exact bootstrap interface.

Эти решения принимаются на Step 2–3 после проверки доступных C++ dependencies и threat boundary.

Обязательные ограничения:

- plaintext password/credential material не хранится;
- собственная криптография не изобретается;
- hardcoded default admin password не допускается;
- bootstrap должен быть явным и безопасным для first-run;
- disabled user не может продолжать получать новый authenticated access;
- ошибки login не должны раскрывать credential secrets;
- secrets/tokens не должны попадать в обычные diagnostics/docs/tests.

External OIDC/OAuth2/SAML/LDAP/Active Directory, MFA и email/password recovery не входят в первый sprint. Будущие identity providers должны иметь возможность отображаться в ту же stable user identity/access model.

## Session и Web boundary

Web Shell должен получить shared authenticated user context, аналогично уже существующим shared Service Hub/project contexts, но user session является security-sensitive state.

Требования:

- unauthenticated state является явным;
- login/logout не создают второй backend transport;
- current user показывается компактно в global Header/user area;
- logout очищает user-sensitive frontend state;
- project context не должен переживать смену пользователя как доверенный контекст;
- session restoration/reload behavior фиксируется тестами;
- browser storage выбирается только после выбора session representation;
- Web не вычисляет окончательные permissions самостоятельно из guessed roles.

Production TLS/Origin/cookie/reverse-proxy policy остаётся отдельной deployment/security задачей, если она не требуется для корректности выбранного browser authentication mechanism. Локальный development `ws://` не объявляется production security baseline.

## Project access semantics

Минимальный real enforcement в этом спринте применяется к существующему Project Manager.

К завершению:

- unauthenticated Project Manager request не должен получать защищённые project data;
- `list-projects` не раскрывает проекты без доступа;
- `get-project` не раскрывает недоступный проект;
- создание проекта требует согласованного global administrative capability;
- изменение проекта требует согласованного project-scoped edit/admin capability;
- user/project permissions проверяются backend-side;
- временная недоступность Users & Access не должна приводить к fail-open;
- access revocation должна быть отражена в Web при следующей authoritative проверке;
- current frontend project context очищается или переводится в понятное недоступное состояние, если пользователь больше не имеет project access;
- Project Manager v1 business payload не получает дублирующие `user_id`/`permissions` fields.

Точные provider error codes фиксируются contract step.

## Режим управления

Control mode входит в roadmap и концепцию, но реального Device Manager/write UI ещё нет.

Поэтому `CORE-005` реализует **security/session baseline режима управления**, а не фиктивное управление оборудованием.

Требования:

- пользователь без effective `control` capability в текущем project scope не может включить режим;
- режим управления относится к authenticated session/user context, а не к Project entity;
- logout/session invalidation выключает режим;
- auto-expiration policy должна быть поддержана или явно зафиксирована как configuration/session rule;
- Web ясно показывает active/inactive state и причину, если включение недоступно;
- Project Manager `edit` operation не подменяется control mode;
- реальные writable metric operations в будущих спринтах будут обязаны проверять одновременно capability и control-mode policy.

Точный timeout/default configuration фиксируется на Step 7 по фактической session model.

## Audit boundary

Концепция требует фиксировать значимые действия пользователя.

До `CORE-011 — Event Hub` нельзя притворяться, что существует общий event/audit pipeline.

В `CORE-005` необходимо как минимум определить и проверить локальное security audit behavior для значимых Users & Access действий, например:

- successful/failed authentication без записи secret material;
- logout/session invalidation;
- изменения пользователей;
- изменения permission sets/assignments;
- включение/выключение control mode.

Технология и durable scope audit выбираются вместе с Users & Access persistence. Публикация security events в будущий Event Hub остаётся отдельной интеграцией.

## Persistence boundary

Users, credential verifiers, permission sets, assignments и необходимая security metadata требуют durable storage.

Конкретная storage technology **не выбирается в плане заранее**.

Требования:

- users/access data переживают restart;
- credential verifier data хранится безопасно;
- schema evolution/versioning предсказуемы;
- storage остаётся внутренней ответственностью Users & Access;
- Project Manager SQLite не становится общей platform database автоматически;
- отсутствие/новый storage имеет явный bootstrap lifecycle;
- ошибки storage/auth должны fail closed там, где от них зависит доступ.

## Ожидаемый результат

К завершению `CORE-005` существует работающая security foundation, которая:

- имеет отдельную backend responsibility Users & Access;
- имеет stable user identity и durable local users/access configuration;
- предоставляет локальную authentication/session boundary;
- предоставляет versioned language-independent external contract;
- интегрируется с существующим Service Hub browser path без второго transport;
- предоставляет backend-authoritative access evaluation;
- защищает реальный Project Manager;
- фильтрует project visibility;
- поддерживает разные project-scoped permissions одного пользователя;
- даёт Web login/logout/current-user UX;
- даёт минимальный administration UI для users/access;
- очищает/перепроверяет project context при user/access changes;
- даёт session baseline control mode;
- фиксирует security-relevant actions без secret leakage;
- подтверждена real browser → Service Hub → Users & Access / Project Manager integration;
- не вводит fake ACL для ещё не существующих ресурсов.

## Критерий завершения

Спринт завершён, если подтверждаются как минимум:

1. Users & Access backend отдельно собирается и тестируется в C++20/WSL.
2. User получает stable opaque identifier.
3. Локальная credential verification не хранит plaintext secrets.
4. Secure first-admin bootstrap не использует hardcoded default password.
5. Users/access configuration durable переживает restart.
6. Permission set/access-profile model не привязан к единственной фиксированной роли.
7. Поддерживаются global и project-scoped assignments.
8. Один user может иметь разные effective permissions в разных projects.
9. Versioned Users & Access external contract зафиксирован language-independent schema/docs.
10. Реальный login создаёт authenticated session/user context.
11. Disabled/invalid user/session получает предсказуемый отказ.
12. Service Hub имеет согласованную authenticated request boundary без второго transport.
13. User ID/permissions не берутся из Project Manager business payload.
14. Project Manager защищён server-side.
15. `list-projects` не раскрывает inaccessible projects.
16. Project create/update проверяют соответствующий backend capability.
17. Users & Access outage не приводит к fail-open.
18. Web имеет login/logout/current-user state.
19. Logout/user change не оставляет старый project context доверенным.
20. Административный Web UI позволяет выполнить минимальные users/access assignments.
21. Недоступные UI actions объясняют причину и не заменяют server-side authorization.
22. Control mode нельзя включить без `control` capability.
23. Logout/session invalidation выключает control mode.
24. Security-sensitive diagnostics/tests не раскрывают passwords/session secrets.
25. Real browser integration подтверждает allowed и denied paths.
26. Restart acceptance подтверждает сохранность durable users/access configuration.
27. Existing Service Hub correlation/cancel/reconnect regression остаётся рабочим.
28. Existing Project Manager durable/restart behavior остаётся рабочим.
29. Concept/architecture boundaries будущих Device/Dashboard/Event Hub не подменены фиктивными records.
30. Выполнен финальный targeted documentation audit.

# Шаги

## Step 1 — Users & Access domain и backend skeleton

### Что делаем

Создаём самостоятельную backend responsibility Users & Access и фиксируем минимальную domain/application model:

- User;
- stable user ID;
- login/display properties;
- enabled/disabled state;
- capabilities;
- permission set/access profile;
- assignment;
- global/project scope;
- access evaluation result/errors.

Определяем deterministic effective-permission semantics, достаточные для первого project-scoped enforcement, не создавая universal ACL engine.

Создаём:

- отдельный CMake/service target;
- domain/application слой;
- repository/storage ports;
- in-memory test implementations;
- unit tests identities/assignments/effective access;
- базовый Linux lifecycle.

На Step 1 ещё нет durable credential storage, Service Hub contract и Web UI.

### Результат

Существует отдельно тестируемая Users & Access domain boundary без придуманного transport/security token format.

### Решение и реализация Step 1

Step 1 создаёт новый `services/users-access/` с отдельными CMake targets `dispatcher_users_access_core`, `dispatcher_users_access` и `dispatcher_users_access_tests`. Production executable на этом шаге не открывает network endpoint и не регистрируется в Service Hub; он подтверждает только самостоятельный Linux lifecycle.

Минимальная модель Step 1:

- `User`: stable opaque `id`, `login`, `display_name`, `enabled`;
- capabilities имеют machine-readable names `view`, `control`, `edit`, `admin`;
- capability names независимы: скрытой иерархии `admin => edit => control => view` нет;
- `PermissionSet` имеет stable opaque ID, имя и набор capabilities;
- `AccessAssignment` связывает user + permission set + один explicit scope;
- реально поддерживаются только `global` и `project(project_id)` scopes;
- explicit deny, nested groups/roles, tenant hierarchy, ABAC и будущие Device/Dashboard scopes не добавляются.

Effective-permission semantics детерминированы:

1. disabled user всегда получает deny и пустой effective set;
2. global assignments участвуют как в global, так и в project evaluation;
3. project assignment участвует только для exact matching project ID;
4. effective capabilities являются union всех matching permission sets;
5. project assignment не даёт global capability;
6. missing user/invalid scope/storage inconsistency возвращают evaluation error, а не fail-open.

`UsersAccessRepository` является внутренним storage port сервиса. In-memory implementation существует только в unit tests; production persistence, credential verifier, bootstrap, authentication/session contract и token representation Step 1 сознательно не выбирает.

Unit tests покрывают stable identity, login conflict/validation, canonical permission sets, global/project union semantics, отсутствие implicit capability hierarchy, disabled-user deny, invalid/missing subject behavior и assignment validation/conflict. Lifecycle tests проверяют clean SIGTERM/SIGINT.

Step 1 завершён commit:

`e8b42e69fecf7079f7b18f5f86fe334308d2579c`

## Step 2 — Durable users/access storage, credentials и bootstrap

### Что делаем

На основании Step 1 выбираем минимальную durable storage technology и established credential hashing dependency.

Реализуем:

- production repository;
- schema versioning;
- durable users/permission sets/assignments;
- credential verifier storage без plaintext;
- explicit secure bootstrap первого admin user;
- enabled/disabled user lifecycle;
- необходимые security audit records;
- restart/reopen tests;
- storage/credential error behavior.

Фиксируем решение storage + password hashing library/parameters в документации с причиной выбора.

Hardcoded default credentials не допускаются.

### Результат

Users & Access configuration переживает restart и имеет безопасный local credential/bootstrap baseline.

### Решение и реализация Step 2

Step 2 выбирает SQLite как **локальное durable storage только Users & Access**. Это не становится общей БД платформы и не меняет storage decision Project Manager/будущих сервисов. SQLite подходит текущему небольшому transactional набору users, permission sets, assignments, credential verifiers и security audit без отдельного database process.

Schema v1 хранит:

- `users`;
- `permission_sets`;
- `access_assignments`;
- `credential_verifiers`;
- `security_audit`.

Schema version фиксируется через `PRAGMA user_version = 1`, foreign keys включены, новая database создаётся автоматически, а schema version новее поддерживаемой отвергается. Production adapter `SqliteUsersAccessRepository` реализует Step 1 repository port, credential/audit ports и атомарный first-admin bootstrap transaction.

Для password verifier Step 2 использует established OpenSSL `EVP_PBE_scrypt`, а не собственную криптографию. Параметры:

- `N = 2^17`;
- `r = 8`;
- `p = 1`;
- 16-byte CSPRNG salt через OpenSSL;
- 32-byte derived digest.

Plaintext password не хранится. Stored verifier содержит algorithm/parameters/salt/digest и остаётся внутренним Users & Access representation; session/token format Step 2 не выбирает.

Explicit first-admin bootstrap:

`dispatcher-users-access --bootstrap-admin <login> <display-name> [database-path]`

Password и confirmation читаются из stdin; в interactive terminal echo отключается. Secret не передаётся через argv/env и не печатается в diagnostics. Bootstrap требует пустое users storage, создаёт enabled user, permission set `Bootstrap administrators` с явными `view/control/edit/admin`, global assignment, scrypt verifier и `bootstrap_admin_created` audit record в **одной SQLite transaction**. Повторный bootstrap отклоняется.

Normal startup:

`dispatcher-users-access [database-path]`

Default database path — `dispatcher-users-access.db`. Storage инициализируется до Linux signal lifecycle; storage failure не допускает запуск сервиса.

Step 2 также добавляет durable `enabled/disabled` user update через application boundary. Полный audit для будущих authentication/admin operations расширяется на следующих шагах вместе с реальными operations; Step 2 не симулирует ещё не существующие login/logout/session actions.

Новые WSL development dependencies:

- `libsqlite3-dev`;
- `libssl-dev`.

Tests покрывают scrypt correct/wrong password verification, SQLite create/reopen, durable users/permission sets/assignments/credentials/audit, disabled fail-closed after reopen, atomic bootstrap, повторный bootstrap refusal, отсутствие plaintext test password в SQLite/output, storage failures и SIGTERM/SIGINT lifecycle.

На Step 2 по-прежнему **нет** Service Hub provider/auth envelope, authentication/session external contract, login operation или Web UI. Это начинается только Step 3.

Step 2 завершён commit:

`3b140d808638bb0c14a70b2aa1df96eb377af197`

## Step 3 — Authentication/session contract

### Что делаем

Фиксируем versioned language-independent Users & Access service contract и exact service address.

Определяем минимальные operations для:

- authentication/login;
- logout/session invalidation;
- current authenticated user/session state;
- access evaluation/authorization, необходимой backend consumers;
- user administration;
- permission set/access assignment administration;
- control-mode session state, если его session representation уже определена на этом шаге.

Выбираем session/token representation и lifecycle:

- generation;
- validation;
- expiration;
- logout/invalidation;
- restart semantics;
- disabled-user behavior.

Добавляем machine-readable schema и contract tests.

Не включаем password hashes/secrets в external responses.

### Результат

Существует стабильный Users & Access auth/session contract, на который можно опереть Service Hub и providers.

### Решение и реализация Step 3

Service address фиксируется как `users-access.v1`. Предметный payload contract документируется в `docs/architecture/users-access-contract.md`, machine-readable definitions — в `services/users-access/protocol/dispatcher/users_access/v1/users_access.schema.json`.

Step 3 сознательно не меняет Service Hub v1 envelope и не регистрирует production Users & Access provider: public/protected operation semantics фиксируются сейчас, а transport authentication propagation принадлежит Step 4. `login` является public operation; остальные operations должны получать subject из trusted authenticated request context и не принимают `user_id`/session token в business payload как доказательство identity.

Session representation:

- opaque bearer token;
- 256 bit CSPRNG entropy;
- wire representation — 64 lowercase hex characters;
- token не содержит user ID/permissions/timestamps;
- SQLite хранит только SHA-256 digest token;
- idle timeout — 30 минут;
- absolute lifetime — 12 часов;
- timeout/validation выполняются server-side;
- successful validation обновляет last activity;
- logout удаляет session;
- sessions durable переживают restart до expiry/revocation;
- disabled user не получает новую session, а existing session fail-closed при следующей validation.

SQLite schema мигрирует `v1 -> v2` добавлением таблицы `sessions` и индекса по user ID. Existing users/access/credentials/audit records не меняются.

Authentication intentionally uses generic invalid-credentials behavior для unknown login, wrong password, missing verifier и disabled user. Для unknown/missing credential path выполняется dummy scrypt verification, чтобы не создавать дешёвый obvious timing path.

Local security audit расширяется событиями `authentication_succeeded`, `authentication_failed`, `session_logged_out`, `session_expired`, `session_rejected_disabled_user`; password/raw bearer token не записываются.

Step 3 реализует `AuthenticationSessionService`, OpenSSL session-token codec и durable session repository; tests покрывают login, generic failure, token format, project-scope access evaluation, restart persistence, idle expiration, disabled-user invalidation, logout и отсутствие raw token в SQLite.

Control mode в session schema Step 3 не добавляется: его реальная representation остаётся Step 7, после проверки authenticated Service Hub/Web boundary. Browser token storage также не выбирается до соответствующего Web/security шага.

Step 3 завершён commit:

`02a2d86e730a6c73ee0c33250bb9d4dc14791681`

## Step 4 — Authenticated Service Hub request boundary

### Что делаем

На основе Step 3 выбираем согласованный способ переноса authentication/session context через текущий browser-compatible Service Hub.

Обновляем только необходимые:

- Service Hub envelope/schema;
- C++ parser/routing structures;
- browser `ServiceHubClient`;
- provider-facing forwarded request model;
- contract/docs/tests.

Требования:

- existing request IDs/correlation/cancel/timeout semantics сохраняются;
- unauthenticated/public auth operations остаются возможны только явно;
- provider не доверяет subject identity из business payload;
- authenticated context невозможно подделать простым `user_id` в payload;
- compatible v1 extension используется только если она действительно compatible; иначе versioning выполняется явно;
- existing test providers/integration paths обновляются минимально и остаются рабочими.

На этом шаге ещё не меняем Project Manager business authorization policy кроме plumbing, необходимого следующему шагу.

### Результат

Browser и backend имеют единый authenticated Service Hub request path без второго transport.

### Решение и реализация Step 4

Service Hub v1 расширяется совместимо optional request field `auth`; endpoint `/v1/ws`, subprotocol `dispatcher.service-hub.v1`, request IDs, correlation, cancellation и timeout semantics не меняются. Текущий transport auth shape — `{ "type": "session", "token": "<64 lowercase hex>" }`. Malformed auth возвращает `hub.invalid_request`.

Hub проверяет только transport shape и переносит auth provider отдельно от business `payload`. Он не декодирует token, не создаёт trusted `user_id`/roles/permissions и не принимает authorization decisions. Наличие syntactically valid auth не является доказательством действующей session: protected provider обязан выполнить authoritative validation через Users & Access security boundary.

Browser `ServiceHubClient.request()` получает optional per-request `auth` без выбора global/persistent browser token storage. Public `users-access.v1/login` отправляется без auth.

Users & Access получает production reconnecting Service Hub provider `users-access.v1`. На Step 4 реально подключены session-core operations `login`, `logout`, `current-session`, `evaluate-access`; protected operations валидируют forwarded bearer через `AuthenticationSessionService`, а `user_id` внутри business payload не может аутентифицировать caller. Contract-defined administration operations остаются зарезервированы для Step 6 и на Step 4 не считаются production-ready admin API.

Real interprocess test поднимает Service Hub + Users & Access на temporary SQLite, bootstrap первого admin через stdin и проверяет public login, authenticated current-session/evaluate, forged subject без auth, malformed auth, logout/revocation и clean shutdown без secret leakage. Existing Service Hub request/response test дополнительно подтверждает auth forwarding при сохранении parallel correlation.

Project Manager authorization policy не меняется на Step 4; он будет использовать эту boundary на Step 5.

Step 4 завершён commit:

`eb5f876e4a35dcbc5b5597e456a1197cc0d9dd1b`

## Step 5 — Project Manager authorization enforcement

### Что делаем

Интегрируем реальный Project Manager с Users & Access security boundary.

Проверяем:

- unauthenticated access;
- filtered `list-projects`;
- allowed/denied `get-project`;
- global permission для create;
- project-scoped permission для update;
- disabled/expired session;
- revoked project access;
- Users & Access unavailable/error => fail closed;
- provider reconnect и auth requests после reconnect.

Не добавляем user/role/auth fields в Project v1 model.

Web project context остаётся frontend context; access определяется backend.

### Результат

Первый реальный business service платформы защищён backend-authoritative authorization.

### Решение и реализация Step 5

Project Manager сохраняет существующий `project-manager.v1` business contract и не получает `user_id`, roles, permissions или auth token внутри Project payload. Все v1 operations теперь требуют forwarded Service Hub session auth и выполняют authoritative access evaluation через `users-access.v1/evaluate-access`.

Authorization policy Step 5:

- `create-project` требует global `admin`;
- `list-projects` отдаёт все проекты при global `view`, иначе backend-side фильтрует список по project-scoped effective `view`;
- `get-project` требует effective `view` для указанного project ID;
- `update-project` требует effective `edit` или `admin` для указанного project ID;
- capabilities остаются независимыми: `admin` не создаёт скрытый `view`/`edit` hierarchy.

Project Manager использует отдельное client-role Service Hub connection для Users & Access и не читает его SQLite напрямую. Authorization connection создаётся лениво, переиспользуется и после transport failure один раз восстанавливается для повторной evaluation. `auth.invalid_session` и `auth.session_expired` возвращаются caller как session errors; deny — как `access.forbidden`; недоступная/неразбираемая security dependency преобразуется в `project.authorization_unavailable`. Business operation при такой ошибке не выполняется.

Integration coverage подтверждает unauthenticated deny, filtered project visibility, allowed/denied get/update/create, project-scoped admin без implicit capability hierarchy, Users & Access outage/recovery, access revocation, disabled/expired session и Service Hub reconnect. Existing Project Manager durable behavior сохраняется.

Step 5 завершён functional commit:

`ebe98d1f16e55d4024438300c1670ec3a19b1d72`

Documentation sync после Step 5 зафиксирован commit:

`ef0b94ce88af77a7032b7e34f1d7a141cf16cd60`

Этот commit был baseline для Step 6A.

## Step 6 — Web login, current user и access administration

### Локальное разбиение Step 6

При входе в Step 6 подтверждено, что administration operation names/payloads уже зафиксированы `users-access.v1`, но production provider Steps 1–5 намеренно возвращал для них staged error. Поэтому Step 6 разделён без изменения его цели и без изменения roadmap-level sprint composition:

- **Step 6A — Users & Access administration backend**: довести уже зарезервированные v1 operations до реального backend application/transport path, сохранить global `admin` enforcement и проверить реальный Service Hub transport;
- **Step 6B — Web authenticated context и administration UI**: добавить browser session restoration, login/logout/current-user UX, authenticated request propagation и минимальный admin destination поверх Step 6A API;
- **Step 6C — administration security audit completion**: завершить уже обязательный для CORE-005 локальный audit значимых administration mutations без изменения Web/auth contract.

Это техническое разбиение одного согласованного шага, а не новый sprint и не расширение product scope. Step 6C выделен отдельно, чтобы не смешивать backend audit semantics с Web-focused Step 6B. Control mode остаётся Step 7.

### Реализация Step 6A

Step 6A добавляет отдельную application boundary `UsersAccessAdministrationService` и SQLite administration store, не меняя schema version v2. Production `users-access.v1` provider реализует все уже зарезервированные administration operations:

- `list-users`;
- `create-user`;
- `set-user-enabled`;
- `set-user-password`;
- `list-permission-sets`;
- `create-permission-set`;
- `list-access-assignments`;
- `assign-access`;
- `remove-access-assignment`.

Все эти operations остаются protected и до business parsing требуют authoritative global `admin` через существующий session/evaluate-access boundary. `login`, `logout`, `current-session` и `evaluate-access` semantics не меняются.

Обычный admin create/reset password использует тот же локальный baseline, что first-admin bootstrap: 15..1024 bytes без composition rule; OpenSSL scrypt verifier остаётся внутренним и plaintext не сохраняется. `create-user` записывает user + credential verifier атомарно одной SQLite transaction.

SQLite administration store открывает ту же service-local database отдельным FULLMUTEX connection, требует уже инициализированную schema v2 и не создаёт новую общую persistence boundary. Existing `SqliteUsersAccessRepository` остаётся source of truth для session/domain paths.

Unit test покрывает atomic user+credential creation, duplicate login, password replacement, enable state, permission sets и assignment add/list/remove. Real interprocess integration поднимает Service Hub + Users & Access и проверяет unauthenticated deny, global-admin operations, non-admin `access.forbidden`, create/list user, permission set, assignment lifecycle, password reset и disabled-user login.

Расширение security audit event taxonomy для create/reset/permission-set/assignment mutations остаётся обязательным completion item текущего `CORE-005` и должно быть закрыто до Sprint acceptance; Step 6A не подменяет его фиктивными event names.

Step 6A завершён commit:

`04e83879c73e298d1eac61acbd8e861f0ba5988d`

### Решение и реализация Step 6B

Step 6B вводит shared browser authenticated user context поверх уже существующего одного `ServiceHubClient`/WebSocket. Низкоуровневый Service Hub client и wire protocol не меняются.

Browser session policy:

- raw bearer хранится только в `sessionStorage` текущей browser session под ключом `dispatcher.user-session.v1`;
- `localStorage`, cookie и self-contained user/permissions token не вводятся;
- browser хранит только opaque bearer и не считает локальный user/permission snapshot доказательством доступа;
- reload с сохранённым bearer выполняет authoritative `users-access.v1/current-session`;
- `auth.invalid_session` / `auth.session_expired` очищают локальный bearer и authenticated React state;
- temporary transport/provider failure не уничтожает bearer автоматически и допускает явный retry restoration;
- logout пытается invalidate server-side session и в любом случае очищает локальный bearer/user-sensitive frontend state.

`BrowserSessionServiceHubClient` является wrapper над существующим shared client: public `users-access.v1/login` остаётся без auth, а остальные protected requests автоматически получают текущее `{type: "session", token}`. При late error старого request wrapper не очищает уже заменённую новую session. Второй WebSocket не создаётся.

React `UserSessionProvider` владеет состояниями unauthenticated/restoring/authenticated, current user и authoritative global effective capabilities. После `current-session` global capabilities запрашиваются через `evaluate-access` и используются только для presentation/navigation; backend остаётся security boundary.

Web Shell получает:

- `/login` и login form;
- compact current-user area в global Header и logout;
- protected `/projects`: unauthenticated caller видит login gate, authenticated Project Manager requests автоматически получают bearer;
- `/access` только как реально полезный administration destination для authenticated global admin; non-admin при прямом переходе получает явный отказ без имитации доступа;
- Users & Access admin UI для users, enabled state/password reset, permission sets и global/project assignments; project assignment принимает explicit project ID и не предполагает, что global admin автоматически имеет project `view`;
- no new npm dependencies/router/state-manager.

Project context остаётся navigation context в `sessionStorage`, но теперь связан с authenticated user lifecycle: unauthenticated/logout/user change очищают его, reload ждёт session restoration, а authoritative `project.not_found` или `access.forbidden` при revalidation удаляют больше недоступный project context. Temporary transport errors не превращаются в доказательство потери доступа.

Backend-independent Playwright smoke проверяет public shell/login gating. Existing real Project Manager browser runner в Step 6B по-прежнему поднимает только Hub + Project Manager и проверяет unauthenticated login gate; полноценный authenticated multi-process browser path с Users & Access остаётся Step 7, как и было запланировано.

Step 6B завершён commit:

`ccde3a262d92ace53069d6e7740108b84f14aad9`

### Решение и реализация Step 6C

Step 6C завершает уже обязательный local security audit administration mutations без нового wire operation, schema migration или Event Hub imitation. Существующая SQLite `security_audit` остаётся единственным durable audit storage текущего Users & Access baseline.

Administration taxonomy расширяется событиями:

- `user_created`;
- `user_enabled` / `user_disabled`;
- `user_password_reset`;
- `permission_set_created`;
- `access_assignment_added`;
- `access_assignment_removed`.

Authenticated actor определяется production provider только из authoritative validated session после global `admin` enforcement и передаётся внутрь administration application boundary отдельно от business payload. Для user и assignment mutations `subject_user_id` содержит target user. Для `permission_set_created` user-specific subject оставляется пустым — permission-set ID не записывается в поле с ложной user semantics.

Mutation paths выполняются через `SqliteUsersAccessAdministrationStore` как одна `BEGIN IMMEDIATE ... COMMIT` transaction вместе с audit insert. Это относится к create user + credential, enable/disable, password reset, permission-set creation и assignment add/remove. Audit write failure приводит к rollback всей mutation и `access.storage_error`; failed validation/conflict и повторный enable-state no-op не создают successful-mutation event. Plaintext password/raw bearer в audit не записываются. SQLite schema остаётся v2.

Administration test проверяет event order, actor/subject semantics, injected timestamps и rollback через test-only SQLite trigger, принудительно отклоняющий audit insert. Existing session/bootstrap audit parser расширяется новыми event names и сохраняет backward compatibility с уже записанными событиями.

После проверенного Step 6C SHA следующим будет Step 7 — control mode и real security integration.

### Что делаем

Расширяем Web Shell реальным user context:

- unauthenticated/login state;
- login form;
- logout;
- current user в compact global user area;
- session restoration согласно Step 3;
- очистка user-sensitive/project context при logout/user change;
- понятная обработка invalid/expired session.

Добавляем минимальный Users & Access administration destination только для реально разрешённого admin:

- users;
- enabled/disabled state;
- permission sets/access profiles;
- global/project assignments;
- credential set/reset operation только в пределах contract baseline.

Web использует shared Service Hub client, а не новый WebSocket.

UI отражает effective access, но не считается security boundary.

### Результат

Пользователь реально входит в Web Shell, видит свой контекст и администратор может настроить минимальные права.

## Step 7 — Control mode и real security integration

### Что делаем

Добавляем session baseline control mode:

- enable/disable;
- проверка `control` capability в текущем project context;
- expiration rule;
- logout/session invalidation reset;
- понятное Web state/reason.

Не создаём fake Device controls.

Затем строим real integration/acceptance path:

`Browser → Service Hub → Users & Access → Project Manager → durable storages`

Проверяем как минимум:

- bootstrap/login;
- current user;
- два users с разными project permissions;
- project list filtering;
- allowed/denied get/update/create;
- project context behavior при access change/logout;
- disabled user/session invalidation;
- control mode allowed/denied/expiration;
- service restart/re-registration;
- Users & Access unavailable => fail closed;
- отсутствие secret leakage;
- существующие Service Hub/Project Manager regressions.

### Результат

Security boundary подтверждена реальными процессами и browser UI, а не только unit tests.

## Step 8 — Sprint acceptance, итоговый отчёт и documentation audit

### Что делаем

Новых product features не добавляем, кроме исправлений, необходимых для критериев завершения.

Выполняем полный acceptance:

- C++ Users & Access build/tests;
- persistence/credential/session tests;
- Service Hub regression;
- Project Manager regression;
- Web typecheck/unit/build/browser smoke;
- generic Service Hub browser integration;
- real Users & Access + Project Manager browser security integration;
- restart/fail-closed scenarios.

Проводим targeted documentation audit:

- root `README.md`;
- `docs/README.md`;
- architecture baseline;
- Service Hub contract;
- Project Manager contract;
- новый Users & Access contract;
- `docs/concept/07-users-access.md`;
- `docs/concept/10-web-ui.md`;
- relevant service/Web README;
- roadmap;
- chat context;
- sprint report.

Заполняем итоговый отчёт, deviations, known limitations и functional commit SHAs.

Closure commit не обязан рекурсивно содержать собственный SHA внутри отчёта.

### Результат

`CORE-005 — Users & Access` закрыт проверенным documentation closure commit и даёт security foundation для `CORE-006 — Device Manager`.

# Что сознательно не входит в CORE-005

- Device Manager records и device-specific ACL;
- Dashboard/Mimic ACL;
- Package Manager permission details до его sprint;
- Event Hub publication security audit events;
- email/password recovery workflow;
- MFA;
- OAuth2/OIDC/SAML;
- LDAP/Active Directory;
- external identity-provider federation;
- service accounts/API keys для внешних интеграций;
- impersonation;
- organization/tenant hierarchy;
- nested permission groups;
- arbitrary ABAC expression language;
- production-final rate limiting/lockout policy;
- production TLS certificate management;
- production reverse proxy;
- production Origin policy;
- HA/clustering/distributed session store;
- universal policy engine;
- fake equipment control UI ради проверки control mode.

Если какая-либо из этих возможностей окажется необходимой для выполнения критерия завершения, она должна быть добавлена в план явным решением, а не скрытым scope expansion.
