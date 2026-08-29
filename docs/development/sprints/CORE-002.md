# CORE-002 — Service Hub

## Статус

**Завершён после успешной проверки Step 8.**

Этап: `L1-01 — Ядро платформы`.

Базовая точка перед началом спринта:

`d9d55d7757ae417b10f855a63ab7f94a1d8645e5`

Этот файл является планом спринта и после завершения будет дополнен итоговым отчётом.

## Цель

Создать второй самостоятельный backend-сервис платформы — Service Hub — как общий адресный механизм request/response между независимыми сервисами.

Service Hub должен позволить клиенту обратиться к конкретному сервису через общую системную границу, не подключаясь к внутренним C++ классам этого сервиса и не зная его внутреннюю реализацию.

Следующий спринт `CORE-003 — Web Shell` должен иметь конкретный клиентский путь к backend через Service Hub.

## Архитектурная граница спринта

Service Hub отвечает за адресное request/response взаимодействие.

Он не должен превращаться в:

- Data Hub для realtime-метрик;
- Event Hub для событий и аварий;
- хранилище бизнес-данных;
- Device Manager;
- систему пользователей и прав;
- универсальный контейнер бизнес-логики всех сервисов.

Service Hub маршрутизирует запрос к нужному provider/service и возвращает ответ клиенту.

Предметный смысл конкретной операции принадлежит целевому сервису. Service Hub не должен знать, что означает, например, «получить устройство», «сохранить проект» или «изменить настройку».

Конкретный транспорт и формат сериализации **не считаются выбранными заранее**. gRPC Data Hub не переносится на Service Hub автоматически.

## Требования, влияющие на выбор транспорта

В `CORE-002 / Step 2` решение должно учитывать одновременно:

- C++ backend-сервисы;
- реальную межпроцессную границу;
- request/response;
- адресацию целевого сервиса и операции;
- корреляцию запроса и ответа;
- ошибки и timeout;
- возможность provider отключиться и подключиться снова;
- пригодность клиентской границы для следующего `CORE-003 — Web Shell`;
- языковую независимость межсервисного контракта;
- разумную сложность реализации и сопровождения.

Не нужно выбирать технологию только потому, что она уже используется Data Hub.

## Ожидаемый результат

К завершению спринта должен существовать отдельный Service Hub, который:

- отдельно собирается и тестируется в Linux/WSL;
- имеет формальный language-independent внешний контракт;
- позволяет provider зарегистрировать адресуемый сервис;
- принимает request от независимого клиента;
- маршрутизирует request к нужному provider;
- возвращает response исходному клиенту;
- корректно различает несколько одновременных запросов;
- предсказуемо обрабатывает неизвестный сервис, timeout и отключение provider;
- позволяет provider отключиться и зарегистрироваться снова;
- имеет минимальный клиентский путь, пригодный для следующего Web Shell;
- корректно запускается и останавливается;
- не содержит пользовательскую авторизацию и предметные API будущих сервисов.

## Критерий завершения

Спринт завершён, если через реальную межпроцессную границу подтверждается сценарий:

1. Запускается Service Hub.
2. Независимый test provider регистрирует тестовый адресуемый сервис.
3. Независимый client отправляет request этому сервису через Service Hub.
4. Provider получает request и формирует response.
5. Client получает соответствующий response.
6. Несколько одновременных request/response не смешиваются между собой.
7. Запрос к неизвестному сервису завершается определённой ошибкой.
8. После отключения provider запрос не зависает бесконечно и завершается определённым результатом.
9. Provider может подключиться/зарегистрироваться повторно, после чего маршрут снова работает.
10. Клиентская граница, выбранная для будущего Web Shell, подтверждена минимальной автоматической или интеграционной проверкой без создания самого React-приложения.
11. Service Hub корректно запускается, останавливается и проходит обязательные тесты.
12. В конце проведена обязательная ревизия актуальности связанной документации.

# Шаги

## Step 1 — Каркас Service Hub и отдельная сборка

### Что делаем

Создаём минимальный самостоятельный C++ service:

- отдельный executable;
- отдельную core/test цель;
- структуру `services/service-hub/`;
- минимальное приложение и lifecycle-каркас, достаточный для дальнейших шагов;
- подключение к существующему CMake-проекту без лишней пересборки Data Hub.

Используем уже подтверждённый общий backend baseline:

- C++20;
- CMake 3.20+;
- Ninja;
- CTest;
- Linux/WSL.

На этом шаге не выбираем transport Service Hub и не реализуем маршрутизацию.

### Результат

Service Hub отдельно собирается и имеет минимальный автоматический test target.

Step 1 завершён commit:

`a06d9cdd7a6c52197b6657c282ed89b745a799da`

## Step 2 — Модель взаимодействия, транспорт и внешний контракт

### Что делаем

Определяем минимальную модель Service Hub:

- идентификатор/address целевого сервиса;
- идентификатор операции;
- request;
- response;
- способ correlation;
- представление ошибок;
- timeout/cancellation semantics;
- минимальную модель регистрации provider.

После этого выбираем конкретный transport и serialization.

При выборе отдельно проверяем пригодность для:

- C++ backend;
- следующего Web Shell;
- независимых процессов;
- language-independent контрактов.

Если выбранная технология требует отдельной browser-facing границы или gateway, это должно быть явно определено в этом же шаге, а не оставлено скрытой проблемой для `CORE-003`.

Не добавляем пользовательские токены, роли, проекты или permission model: они относятся к будущим спринтам.

### Результат

В репозитории есть формальный контракт Service Hub и документированное решение по transport/serialization, достаточное для практической реализации следующих шагов.

### Решение Step 2

Для Service Hub v1 выбраны:

- WebSocket как единый двусторонний transport;
- UTF-8 JSON как application serialization;
- endpoint path `/v1/ws`;
- WebSocket subprotocol `dispatcher.service-hub.v1`;
- JSON Schema общего envelope-контракта.

Provider регистрирует один service address на отдельном provider connection. Client connection может иметь несколько одновременных requests. Service Hub маршрутизирует по `service`, передаёт `operation` и opaque JSON `payload`, а correlation выполняет через request IDs.

Timeout, cancellation, provider disconnect/reconnect и Hub error codes определены контрактом сейчас и реализуются в соответствующих следующих шагах.

Контракт:

`docs/architecture/service-hub-contract.md`

Machine-readable schema:

`services/service-hub/protocol/dispatcher/service_hub/v1/service_hub.schema.json`

Для C++ WebSocket implementation выбран Boost.Asio + Boost.Beast как networking foundation. Конкретная JSON parsing library остаётся внутренней деталью реализации и не является частью межсервисного контракта.

## Step 3 — Регистрация provider и таблица маршрутизации

### Что делаем

Реализуем минимальный механизм, через который provider становится доступен по адресу Service Hub.

Нужно определить и проверить:

- корректную регистрацию;
- некорректный service address;
- поведение при конфликте/повторной регистрации;
- удаление маршрута после отключения;
- повторную регистрацию после reconnect.

Не создаём каталог бизнес-возможностей, Package Manager или постоянный service registry.

### Результат

Service Hub знает, как направить request текущему provider тестового сервиса.

Step 3 завершён commit:

`e04227b2f05d2ceb42a42ae2e6851d14905602f0`

Реализован потокобезопасный `ProviderRegistry` с правилами v1: один active provider на service, один service на connection, удаление route при disconnect и возможность re-register новым connection.

## Step 4 — Реальный request/response маршрут

### Что делаем

Подключаем routing к внешнему контракту.

Проверяем реальную цепочку:

`Client → Service Hub → Test provider → Service Hub → Client`

Client и provider должны взаимодействовать через транспортную границу, а не через прямой вызов внутренних C++ классов.

Service Hub не интерпретирует предметный payload тестовой операции сверх того, что необходимо универсальному envelope/contract.

### Результат

Один независимый client получает response от независимого provider через Service Hub.

### Реализация Step 4

Добавляется реальная loopback WebSocket-проверка с двумя независимыми соединениями:

`Client WebSocket → Service Hub → Provider WebSocket → Service Hub → Client WebSocket`

Service Hub:

- принимает provider registration через v1 endpoint/subprotocol;
- находит provider через `ProviderRegistry`;
- создаёт Hub-scoped request ID;
- передаёт `service`, `operation`, opaque JSON `payload` и timeout provider;
- принимает provider response;
- восстанавливает исходный client request ID;
- возвращает response клиенту.

Для C++ transport используются Boost.Asio + Boost.Beast. Для внутреннего JSON parsing/serialization используется `json-c`; это implementation detail и не меняет внешний контракт.

Полная проверка нескольких одновременно активных requests остаётся Step 5.

Step 4 завершён commit:

`53ffaa830dcd4aa9908a43ab2b6d2f83cb940c8e`

## Step 5 — Correlation и параллельные запросы

### Что делаем

Проверяем, что Service Hub остаётся request/response механизмом при нескольких активных запросах.

Минимально:

- несколько одновременных requests;
- response возвращается именно исходному request/client;
- один медленный request не должен подменять response другого;
- request identifiers/correlation не конфликтуют в проверяемом сценарии.

Не строим сложный production scheduler или распределённую очередь.

### Результат

Параллельные request/response корректно сопоставляются.

### Реализация Step 5

Client session больше не блокируется в ожидании одного provider response. Она продолжает принимать следующие WebSocket messages, а завершённые responses возвращаются через сериализованную outbound queue этой же session.

Для correlation используется единая pending table:

`provider-scoped request ID -> client session + client request ID + deadline`

Service Hub не создаёт отдельный worker thread на каждый request. Один timeout-monitor обслуживает deadlines всех активных requests.

Автоматическая loopback WebSocket-проверка подтверждает:

- два одновременно активных requests на одном client connection;
- ответы provider в обратном порядке;
- правильное восстановление исходных client request IDs;
- быстрый request не блокируется медленным request, который завершается `hub.timeout`;
- одинаковый client request ID разрешён одновременно на двух разных client connections;
- таким requests назначаются разные Hub-scoped IDs и responses не смешиваются.

Полный client cancellation и provider-disconnect/reconnect lifecycle остаются Step 7.

Step 5 завершён commit:

`67010fcff69e243f18c21d7909979580cb4d82dc`

## Step 6 — Клиентская граница для Web Shell

### Что делаем

Подтверждаем, что выбранный внешний клиентский путь Service Hub реально пригоден следующему `CORE-003 — Web Shell`.

React-приложение в этом спринте не создаём.

Нужна минимальная проверка клиентской границы выбранной технологии:

- browser-compatible protocol напрямую;
- либо явно определённый и реализованный минимальный gateway, если основной внутренний transport не может использоваться Web-клиентом напрямую.

Web-клиент не должен получать прямой доступ к внутренней реализации provider.

### Результат

У `CORE-003` есть конкретная рабочая точка входа в backend через Service Hub, а не только архитектурное обещание.

### Реализация Step 6

Отдельный gateway не требуется: Service Hub v1 уже является напрямую browser-compatible WebSocket boundary.

Добавляется CTest `service-hub.browser-boundary`, который использует реальный loopback Service Hub и проверяет browser-shaped connection:

- стандартный WebSocket Upgrade;
- `Origin` header;
- subprotocol `dispatcher.service-hub.v1`;
- отсутствие custom application HTTP headers;
- явное согласование subprotocol сервером;
- JSON request/response через зарегистрированный test provider.

Этим подтверждается конкретная точка входа будущего Web Shell:

```typescript
const socket = new WebSocket(
  serviceHubUrl,
  "dispatcher.service-hub.v1"
);
```

React-приложение, authentication, production Origin policy и TLS в Step 6 не добавляются.

Step 6 завершён commit:

`494b1f55f9570993241006ef379f3de519696747`

## Step 7 — Lifecycle, ошибки и переподключение

### Что делаем

Проверяем необходимое поведение самостоятельного сервиса:

- запуск и остановку Service Hub;
- неизвестный service address;
- некорректный request;
- timeout;
- provider disconnect;
- provider reconnect;
- client disconnect/cancellation там, где применимо;
- базовые диагностические сообщения.

Не создаём большую production observability-систему.

### Результат

Service Hub предсказуемо работает не только в happy-path сценарии.

### Реализация Step 7

Самостоятельный executable теперь запускает реальный `ServiceHubServer`, принимает необязательный listen address и остаётся активным до SIGINT/SIGTERM.

Используется тот же проверенный Linux signal pattern, что и в Data Hub: SIGINT/SIGTERM блокируются до создания worker threads и обрабатываются синхронно через `sigwait()`.

Transport дополнен уже определённой v1-семантикой:

- client `cancel` завершает request как `hub.cancelled`;
- Hub best-effort передаёт provider `cancel` при client cancel, client disconnect и timeout;
- provider disconnect завершает связанные active requests как `hub.provider_unavailable`;
- route удаляется после provider disconnect;
- новый provider connection может зарегистрировать тот же service;
- поздний provider response после timeout/cancel игнорируется и не разрушает provider connection.

Добавляются CTest-проверки:

- `service-hub.lifecycle-and-errors`;
- `service-hub.signal-term`;
- `service-hub.signal-int`.

Lifecycle/error test дополнительно проверяет unknown service, invalid request, timeout, reconnect, client disconnect и bounded shutdown с активным request.

Step 7 завершён commit:

`ba66ff6c3625c61adf5d6b2c1a4d89fd7a1a8e72`

## Step 8 — Проверка спринта, итоговый отчёт и documentation audit

### Что делаем

Новых функций не добавляем, кроме исправлений, необходимых для критериев завершения.

Выполняем единый sprint acceptance сценарий:

1. Hub стартует.
2. Test provider регистрируется.
3. Client выполняет успешный request/response.
4. Проверяются параллельные requests.
5. Unknown service возвращает определённую ошибку.
6. Provider отключается, и запрос завершается предсказуемо.
7. Provider подключается повторно, и request/response снова работает.
8. Проверяется клиентская граница для будущего Web Shell.
9. Проверяется корректная остановка Hub.

После этого:

- выполняем обязательную сборку и тесты;
- исправляем найденные проблемы в рамках `CORE-002`;
- заполняем итоговый отчёт в этом файле;
- проводим обязательную целевую ревизию документации;
- фиксируем documentation-closure commit;
- пользователь возвращает SHA;
- SHA проверяется в репозитории и становится финальным baseline спринта.

### Результат

Все критерии `CORE-002` подтверждены, документация синхронизирована, следующий спринт может опираться на реальный Service Hub.

### Реализация Step 8

Добавлен отдельный CTest `service-hub.sprint-acceptance` через реальную loopback WebSocket boundary.

Один acceptance scenario подтверждает цепочку:

1. Service Hub стартует на ephemeral loopback port.
2. Test provider регистрирует `test.acceptance`.
3. Browser-shaped client подключается к `/v1/ws` с `Origin` и subprotocol `dispatcher.service-hub.v1`.
4. Выполняется успешный request/response.
5. Два requests одновременно находятся в работе, provider отвечает в обратном порядке, а client получает правильные responses.
6. Unknown service возвращает `hub.unknown_service`.
7. Provider отключается во время active request, client получает `hub.provider_unavailable`.
8. Новый provider регистрирует тот же service после reconnect.
9. После reconnect request/response снова работает.
10. Shutdown с активным long-running request остаётся bounded.

Step 8 не добавляет новую production-функциональность и не меняет Service Hub v1 contract.

# Что сознательно не входит в CORE-002

- React/Web Shell;
- реальные API Project Manager;
- реальные API Device Manager;
- пользователи;
- authentication;
- authorization;
- роли и permission sets;
- project access checks;
- control mode;
- Event Hub;
- события и аварии;
- Data Hub forwarding/proxy как отдельная функция;
- Driver Runtime;
- Package Manager service discovery;
- бизнес-специфичная схема операций всех будущих сервисов;
- постоянное хранение registry;
- кластеризация;
- high availability;
- load balancing между несколькими экземплярами одного provider;
- production-final TLS/security модель, если она не требуется выбранному минимальному transport для корректной локальной проверки;
- production tracing/metrics/log aggregation;
- deployment orchestration.

Если во время спринта обнаружена новая задача, не необходимая для его критерия завершения, она не добавляется автоматически.

# Итоговый отчёт

## Фактически реализовано

В `CORE-002` реализован самостоятельный Service Hub на C++20.

Внешняя граница v1:

- WebSocket;
- UTF-8 JSON text messages;
- endpoint `/v1/ws`;
- subprotocol `dispatcher.service-hub.v1`;
- machine-readable JSON Schema общего envelope.

Runtime-механизмы:

- provider registration по непрозрачному service address;
- один active provider на service и один service на provider connection;
- route removal после provider disconnect;
- request addressing через `service` + `operation`;
- opaque JSON payload без предметной интерпретации Hub;
- Hub-scoped provider request IDs и client-local request ID namespaces;
- несколько одновременно активных requests;
- корректная correlation при out-of-order responses;
- единая pending table и общий deadline monitor без отдельного OS thread на request;
- timeout;
- client cancellation;
- best-effort provider cancellation при client cancel/disconnect и timeout;
- `hub.unknown_service`, `hub.invalid_request`, `hub.timeout`, `hub.cancelled`, `hub.provider_unavailable` и базовые protocol errors;
- provider disconnect/reconnect;
- игнорирование поздних responses после timeout/cancel;
- напрямую browser-compatible client boundary без обязательного дополнительного gateway;
- самостоятельный executable с optional listen address;
- SIGINT/SIGTERM через blocked signals + synchronous `sigwait()`;
- bounded shutdown и базовые lifecycle diagnostics.

Текущая C++ implementation использует Boost.Asio + Boost.Beast и внутренний `json-c`. Межсервисный контракт от этих библиотек не зависит.

## Выполненные проверки

Финальная clean WSL-проверка выполняется перед commit Step 8 и включает сборку Service Hub и восемь CTest:

- `service-hub.application`;
- `service-hub.provider-registry`;
- `service-hub.request-response`;
- `service-hub.browser-boundary`;
- `service-hub.lifecycle-and-errors`;
- `service-hub.signal-term`;
- `service-hub.signal-int`;
- `service-hub.sprint-acceptance`.

`service-hub.sprint-acceptance` отдельно проверяет полный путь registration → browser-shaped client request → provider response → parallel correlation → unknown service → provider disconnect → reconnect → повторный request/response → bounded shutdown.

Regression-проверки дополнительно покрывают invalid request, timeout, client cancel, provider cancel, поздний response после timeout/cancel, client disconnect cleanup и реальные SIGINT/SIGTERM процесса.

## Отклонения от плана

Существенного расширения scope не было.

Конкретный transport намеренно не был выбран до Step 2. После сравнения требований выбран WebSocket + JSON, а не автоматическое повторение gRPC Data Hub.

Отдельный browser gateway не понадобился: выбранный v1 WebSocket protocol напрямую совместим с browser WebSocket API, что было подтверждено Step 6.

В Step 5 первоначальная последовательная модель client session была заменена на общую pending-correlation table и outbound queues, чтобы выполнить требование параллельных requests без создания отдельного worker thread на каждый request.

В Step 7 Linux signal lifecycle повторно использует уже подтверждённый в Data Hub pattern вместо создания второго несовместимого механизма.

## Известные ограничения

После `CORE-002` сознательно остаются:

- нет authentication и authorization;
- нет user/project/control-mode context;
- нет production Origin policy;
- нет TLS termination / обязательного `wss://` deployment решения;
- provider registry хранится только в памяти и не восстанавливается после restart;
- один active provider на service, без load balancing;
- один service на provider connection;
- нет clustering/high availability;
- нет service discovery через Package Manager;
- нет каталога operations или централизованной предметной schema registry;
- нет production-final observability/log aggregation;
- connection/session threading и очереди не проходили production-scale нагрузочные тесты;
- outbound queue/backpressure policy пока не является production-final;
- нет специально настроенного active heartbeat interval поверх стандартного WebSocket transport;
- browser boundary подтверждена protocol-level integration test, но реальный React Web Shell создаётся только в `CORE-003`.

## Проверка актуальности документации

В Step 8 проведена целевая ревизия документов, которые могли устареть после реализации Service Hub.

Синхронизированы:

- корневой `README.md`;
- `docs/README.md`;
- `docs/architecture/README.md`;
- `docs/architecture/service-hub-contract.md`;
- `docs/development/ROADMAP.md`;
- этот sprint report;
- `services/service-hub/README.md`;
- `docs/context/CHAT_CONTEXT.md`.

Дополнительно устранена неоднозначность контракта для позднего provider response: timeout/cancel semantics требуют его игнорировать, поэтому общий текст про unknown/completed provider IDs уточнён и больше не противоречит этим правилам.

`AGENTS.md` проверен: правила source-of-truth, ZIP overlay, запрета `git diff` и обязательного documentation audit уже актуальны, поэтому изменение файла не требуется.

Concept-документы проверены по области влияния. `docs/concept/10-web-ui.md` не требует изменения: `CORE-002` определил транспортную границу Web Shell, но не изменил согласованную продуктовую модель Web UI.

## Итоговый baseline

Последний подтверждённый implementation baseline перед Step 8:

`ba66ff6c3625c61adf5d6b2c1a4d89fd7a1a8e72`

Финальным baseline `CORE-002` является Step 8 documentation-closure commit, содержащий sprint acceptance test, этот итоговый отчёт и синхронизированные документы.

Его SHA пользователь возвращает после успешной clean WSL-проверки, после чего SHA проверяется в GitHub перед началом `CORE-003`.

Отдельный рекурсивный commit только ради записи SHA самого closure commit внутрь этого файла не создаётся.
