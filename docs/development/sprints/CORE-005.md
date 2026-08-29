# CORE-005 — Users & Access

## Статус

**План спринта. Реализация не начата.**

Этап: `L1-01 — Ядро платформы`.

Проверенный baseline перед планированием:

`29b1f0ea750633cc53cc4e023585835d2b06ad8b`

Это documentation-closure commit завершённого `CORE-004 — Project Manager`.

После фиксации этого плана отдельным commit его SHA должен быть проверен в репозитории. Только после этого начинается Step 1.

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

## Step 6 — Web login, current user и access administration

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
