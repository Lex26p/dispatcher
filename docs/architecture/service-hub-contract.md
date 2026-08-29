# Внешний контракт Service Hub

## Статус

Контракт зафиксирован в `CORE-002 / Step 2`.

Он определяет минимальную versioned transport boundary Service Hub, необходимую для:

- независимых C++ backend-сервисов;
- provider registration;
- адресного request/response;
- будущего Web Shell;
- timeout, cancellation и reconnect semantics.

Контракт намеренно не определяет предметные API Device Manager, Project Manager и других будущих сервисов.

## Выбранная модель

Service Hub использует:

- **WebSocket** как транспорт;
- **UTF-8 JSON** как сериализацию application messages;
- endpoint path `/v1/ws`;
- WebSocket subprotocol `dispatcher.service-hub.v1`;
- JSON Schema `services/service-hub/protocol/dispatcher/service_hub/v1/service_hub.schema.json`.

Один WebSocket text message содержит ровно один JSON document.

Binary WebSocket messages не входят в v1.

## Почему WebSocket

Service Hub одновременно должен:

1. принимать запросы от обычных backend-клиентов;
2. иметь постоянное соединение с provider, чтобы отправлять ему адресованные requests;
3. поддерживать несколько одновременных requests;
4. работать из browser Web Shell без обязательного отдельного transport gateway.

Обычный HTTP/JSON хорошо подходит для browser request/response, но сам по себе не даёт provider удобный обратный канал Hub → provider без polling или второго транспорта.

gRPC удобен для C++ backend, но browser-клиент потребовал бы gRPC-Web boundary/proxy и тем самым отдельный Web-facing transport layer.

WebSocket даёт один двусторонний protocol для обеих сторон.

## C++ implementation

Межсервисный контракт не зависит от C++ библиотеки.

Текущая реализация Service Hub использует Boost.Asio + Boost.Beast как C++ WebSocket/networking foundation.

Для внутреннего JSON parsing/serialization используется `json-c`. Это implementation detail и не является частью внешнего контракта.

## Versioning

Версия v1 фиксируется одновременно:

- в endpoint path `/v1/ws`;
- в WebSocket subprotocol `dispatcher.service-hub.v1`;
- в JSON Schema path.

Client/provider должен запрашивать WebSocket subprotocol:

`dispatcher.service-hub.v1`

Hub принимает только поддерживаемую версию.

Новые несовместимые версии получают отдельный endpoint/subprotocol и не изменяют значение уже опубликованных v1 message types.

## Connection roles

Service Hub различает два типа application connection.

### Client connection

Client отправляет `request` и `cancel`.

Hub отправляет `response` и при connection-level ошибке `protocol_error`.

Один client connection может иметь несколько одновременно активных requests.

### Provider connection

Первым application message provider connection должен отправить `register`.

После успешной регистрации:

- Hub отправляет provider messages `request` и `cancel`;
- provider отправляет Hub messages `response`.

Один provider connection в v1 регистрирует **один service address**.

Если один backend-процесс одновременно предоставляет сервис и сам выполняет запросы через Service Hub, он открывает отдельные provider и client WebSocket connections.

Это сознательно упрощает состояние соединения и routing.

## Service и operation

`service` — глобальный адрес provider/service в Service Hub.

Примеры:

- `device-manager`;
- `project-manager`;
- `test.echo`.

`operation` — имя операции внутри конкретного service.

Примеры:

- `get-device`;
- `list-metrics`;
- `echo`.

Service Hub:

- маршрутизирует по `service`;
- передаёт `operation` provider без предметной интерпретации;
- не знает схему `payload`;
- сравнивает service address как точную строку.

В v1 `service` и `operation`:

- содержат только lowercase ASCII letters, digits, `.`, `_`, `-`;
- начинаются с буквы или цифры;
- имеют длину от 1 до 128 символов.

Предметный provider сам определяет допустимые operations и payload schemas.

## Provider registration

Provider первым message отправляет:

```json
{
  "type": "register",
  "service": "test.echo"
}
```

При успехе Hub отвечает:

```json
{
  "type": "registered",
  "service": "test.echo"
}
```

Правила v1:

- один service имеет не более одного активного provider;
- новая регистрация не вытесняет существующий provider;
- конфликт регистрации завершается `protocol_error` с кодом `hub.service_in_use`;
- после закрытия provider connection route удаляется;
- provider может подключиться снова и зарегистрировать тот же service;
- registry не сохраняется после рестарта Service Hub.

Load balancing и несколько providers одного service не входят в `CORE-002`.

## Client request

Пример:

```json
{
  "type": "request",
  "id": "req-42",
  "service": "test.echo",
  "operation": "echo",
  "payload": {
    "text": "hello"
  },
  "timeout_ms": 5000
}
```

Поля:

- `type` — `request`;
- `id` — correlation ID клиента;
- `service` — адрес provider;
- `operation` — предметная операция provider;
- `payload` — любое JSON value, включая `null`;
- `timeout_ms` — необязательный timeout в миллисекундах.

Если `timeout_ms` отсутствует, используется 5000 ms.

Допустимый диапазон v1:

`1..60000 ms`

Deadline начинается в момент, когда Hub принимает request.

## Request ID и correlation

Client request `id`:

- непрозрачен для Hub;
- должен быть уникален среди **активных requests данного client connection**;
- может быть переиспользован только после завершения предыдущего request с этим ID.

Hub не передаёт client ID provider напрямую.

При маршрутизации Hub создаёт собственный provider-scoped request ID и хранит mapping:

```text
client connection + client request id
    ↔
provider connection + provider request id
```

Provider получает request с Hub-generated `id`.

Это предотвращает collisions между одинаковыми client IDs разных clients.

## Forwarded request

Provider получает тот же logical request envelope:

```json
{
  "type": "request",
  "id": "hub-17",
  "service": "test.echo",
  "operation": "echo",
  "payload": {
    "text": "hello"
  },
  "timeout_ms": 5000
}
```

Для provider поле `id` принадлежит Hub и уникально среди активных requests этого provider connection.

`timeout_ms` сообщает deadline budget запроса. Hub всё равно является владельцем фактического timeout.

## Successful response

Provider отвечает:

```json
{
  "type": "response",
  "id": "hub-17",
  "ok": true,
  "payload": {
    "text": "hello"
  }
}
```

Hub возвращает client response, восстановив исходный client ID:

```json
{
  "type": "response",
  "id": "req-42",
  "ok": true,
  "payload": {
    "text": "hello"
  }
}
```

Service Hub не интерпретирует successful `payload`.

## Error response

Ошибка request представляется тем же `response` envelope:

```json
{
  "type": "response",
  "id": "req-42",
  "ok": false,
  "error": {
    "code": "hub.unknown_service",
    "message": "No active provider is registered for the requested service"
  }
}
```

`error` содержит:

- `code` — machine-readable code;
- `message` — короткое человекочитаемое описание;
- необязательный `details` — любое JSON value.

Prefix `hub.` зарезервирован за ошибками, созданными Service Hub.

Provider-specific errors используют собственные codes без prefix `hub.` и передаются client без предметной интерпретации.

## Hub error codes v1

Минимально определяются:

- `hub.invalid_request` — request envelope некорректен;
- `hub.unknown_service` — active provider для service отсутствует;
- `hub.service_in_use` — provider пытается зарегистрировать уже занятый service;
- `hub.provider_unavailable` — provider отключился во время активного request;
- `hub.timeout` — deadline request истёк;
- `hub.cancelled` — request отменён client;
- `hub.protocol_error` — connection нарушил protocol state;
- `hub.message_too_large` — application message превысил допустимый размер.

Этот список может расширяться совместимо внутри v1 новыми error codes.

## Cancellation

Client может отменить активный request:

```json
{
  "type": "cancel",
  "id": "req-42"
}
```

Hub:

1. помечает request завершённым;
2. отправляет client error response `hub.cancelled`;
3. если request уже передан provider, best-effort отправляет provider `cancel` с provider-scoped ID;
4. игнорирует поздний response provider для отменённого request.

Provider получает:

```json
{
  "type": "cancel",
  "id": "hub-17"
}
```

Provider может использовать cancel для прекращения работы, но отдельный cancel acknowledgement в v1 не требуется.

Если client просто отключился, Hub выполняет ту же internal cancellation для его незавершённых requests, но response клиенту уже не отправляется.

## Timeout

По истечении deadline Hub:

1. завершает client request ошибкой `hub.timeout`;
2. best-effort отправляет `cancel` provider;
3. удаляет correlation mapping;
4. игнорирует поздний provider response.

Provider не может продлить client deadline своим response.

## Provider disconnect

Если provider connection закрывается:

- route удаляется;
- новые requests получают `hub.unknown_service`;
- уже направленные этому provider requests завершаются `hub.provider_unavailable`;
- поздние данные старого connection не могут восстановить route;
- provider должен создать новое connection и выполнить `register` снова.

## Duplicate IDs

Повторное использование client `id`, пока request с таким ID ещё активен, является protocol error, потому что response невозможно однозначно сопоставить.

Hub отправляет `protocol_error` и завершает такое client connection.

Provider response с ID, который Hub никогда не выдавал либо который не относится к допустимому active/recently-finished request, является protocol error.

Поздний provider response для request, уже завершённого timeout или cancellation, является отдельным ожидаемым случаем: он игнорируется согласно разделам `Cancellation` и `Timeout` и не должен разрушать здоровое provider connection.

## Protocol error

Для connection-level ошибки, которую нельзя представить как normal request response:

```json
{
  "type": "protocol_error",
  "error": {
    "code": "hub.protocol_error",
    "message": "Unexpected message for the current connection role"
  }
}
```

После критической protocol error Hub может закрыть WebSocket connection.

## Message limits

Для v1:

- только WebSocket text messages;
- UTF-8 JSON;
- один JSON document на message;
- максимальный application message size — 1 MiB;
- batching нескольких envelopes в одном message не поддерживается.

Ограничение размера относится ко всему JSON message, включая payload.

## WebSocket close behavior

Базовые close codes:

- `1000` — normal shutdown;
- `1002` — protocol error;
- `1003` — unsupported binary/data type;
- `1008` — registration/policy violation;
- `1009` — message too large;
- `1011` — unexpected internal server error.

Application request errors обычно возвращаются через `response`, а не закрытием connection.

## Ping/pong

Стандартные WebSocket ping/pong frames относятся к transport layer и не представлены отдельными JSON messages.

`CORE-002` не вводит собственный активный heartbeat interval или application-level heartbeat protocol. Production heartbeat/reconnect policy может быть добавлена позднее по реальной эксплуатационной необходимости без изменения текущих request/response envelopes.

## Web Shell boundary

Web Shell использует тот же endpoint и тот же v1 client protocol:

```javascript
const socket = new WebSocket(
  "ws://host:port/v1/ws",
  "dispatcher.service-hub.v1"
);
```

Отдельный обязательный gRPC-Web/HTTP gateway для доступа browser к Service Hub не нужен.

Production deployment может использовать `wss://` и reverse proxy, но application protocol v1 от этого не меняется.

Конкретная authentication/origin/TLS policy не определяется `CORE-002 / Step 2`.

### Проверка browser boundary в CORE-002 / Step 6

Прямая browser-facing граница подтверждается отдельным integration test.

Тест открывает WebSocket-соединение в форме, доступной обычному Web-приложению:

- endpoint `/v1/ws`;
- subprotocol `dispatcher.service-hub.v1`;
- стандартный `Origin` header;
- без custom application HTTP headers.

Service Hub должен вернуть `101 Switching Protocols` и явно выбрать subprotocol `dispatcher.service-hub.v1`, после чего через то же соединение выполняется реальный JSON request/response к test provider.

Таким образом, для `CORE-003 — Web Shell` рабочая точка входа уже определена:

```typescript
const socket = new WebSocket(
  serviceHubUrl,
  "dispatcher.service-hub.v1"
);
```

Отдельный обязательный gateway между браузером и Service Hub не требуется.

Production Origin policy, authentication и `wss://`/TLS остаются отдельными будущими решениями.

## Security boundary

`CORE-002` определяет routing transport, но не authentication/authorization.

В v1 текущего спринта envelope не содержит:

- user ID;
- roles;
- permissions;
- project access;
- control mode;
- auth token.

Эти данные нельзя придумывать заранее как generic JSON fields.

Когда будут разработаны Users & Access и Web security boundary, пользовательский контекст будет добавлен согласованным способом без переноса предметной авторизации в providers случайным образом.

## JSON Schema

Machine-readable envelope schema:

`services/service-hub/protocol/dispatcher/service_hub/v1/service_hub.schema.json`

Schema проверяет структуру отдельных messages.

Она не может сама проверить stateful protocol rules, например:

- что `register` является первым provider message;
- что request ID уникален среди active requests;
- что response ID существует;
- что service имеет только одного active provider;
- что message direction соответствует connection role.

Эти правила проверяет реализация Service Hub и интеграционные тесты.

## Реализация CORE-002

К завершению `CORE-002` внешний v1-контракт подтверждён реальными loopback WebSocket integration tests.

Проверены:

- provider registration и routing по `service`;
- успешный request/response с восстановлением исходного client request ID;
- несколько одновременно активных requests и out-of-order responses;
- независимые client-local request ID namespaces;
- `hub.unknown_service` и `hub.invalid_request`;
- timeout и best-effort provider cancel;
- client cancel;
- provider disconnect с `hub.provider_unavailable`;
- route removal и повторная регистрация provider после reconnect;
- игнорирование позднего response после timeout/cancel;
- browser-shaped WebSocket handshake с `Origin` и согласованием subprotocol;
- bounded shutdown и остановка процесса по SIGINT/SIGTERM.

Authentication/authorization, production Origin/TLS policy, persistent registry, load balancing и high availability остаются за границами `CORE-002`.
