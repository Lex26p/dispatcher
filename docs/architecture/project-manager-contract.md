# Внешний контракт Project Manager

## Статус

Контракт фиксируется в `CORE-004 / Step 3`.

Он определяет первую versioned language-independent boundary Project Manager поверх уже существующего Service Hub v1.

Project Manager не вводит новый транспорт и не меняет Service Hub envelope.

## Transport boundary

Project Manager является provider Service Hub:

- Service Hub endpoint: `/v1/ws`;
- WebSocket subprotocol: `dispatcher.service-hub.v1`;
- provider service address: `project-manager.v1`.

Client request использует обычный Service Hub `request` envelope:

```json
{
  "type": "request",
  "id": "req-1",
  "service": "project-manager.v1",
  "operation": "list-projects",
  "payload": {},
  "timeout_ms": 5000
}
```

Project Manager определяет только `operation`, `payload`, successful response payload и provider-specific error codes.

## Versioning

Версия Project Manager contract зафиксирована service address:

`project-manager.v1`

Несовместимая будущая версия должна получить другой service address. Совместимые дополнения v1 не меняют семантику уже опубликованных operations.

Machine-readable payload definitions находятся в:

`services/project-manager/protocol/dispatcher/project_manager/v1/project_manager.schema.json`

## Project

Project v1 содержит только:

```json
{
  "id": "4c9b...",
  "name": "Объект 1",
  "description": "Основной производственный объект"
}
```

Поля:

- `id` — стабильный opaque identifier проекта;
- `name` — изменяемое человекочитаемое имя;
- `description` — изменяемое описание, может быть пустой строкой.

`id` не выводится из имени. Duplicate project names разрешены.

Project v1 не содержит parent project, Dashboard IDs, Device IDs, ACL, user ID или других future-resource ownership fields.

Внутренняя validation Project Manager ограничивает UTF-8 payload `name` 256 байтами, `description` — 4096 байтами. Эти byte limits не выражаются через JSON Schema `maxLength`, чтобы не подменять байтовую семантику количеством Unicode code points.

## Operations

### `create-project`

Request payload:

```json
{
  "name": "Объект 1",
  "description": "Описание"
}
```

- `name` обязателен и должен быть string;
- `description` необязателен; отсутствие означает пустую строку;
- неизвестные поля не принимаются.

Successful payload:

```json
{
  "project": {
    "id": "opaque-id",
    "name": "Объект 1",
    "description": "Описание"
  }
}
```

### `list-projects`

Request payload — пустой object:

```json
{}
```

Successful payload:

```json
{
  "projects": [
    {
      "id": "opaque-id",
      "name": "Объект 1",
      "description": "Описание"
    }
  ]
}
```

Порядок списка в v1 не является отдельным контрактным обещанием сортировки.

### `get-project`

Request payload:

```json
{
  "id": "opaque-id"
}
```

`id` должен быть непустой строкой.

Successful payload совпадает с `create-project`:

```json
{
  "project": {
    "id": "opaque-id",
    "name": "Объект 1",
    "description": "Описание"
  }
}
```

### `update-project`

Request payload:

```json
{
  "id": "opaque-id",
  "name": "Новое имя",
  "description": "Новое описание"
}
```

Все три поля обязательны. Update заменяет mutable `name` и `description`; `id` остаётся неизменным.

Successful payload содержит обновлённый `project`.

Удаление проекта не входит в v1 baseline `CORE-004 / Step 3`.

## Provider errors

Provider-specific errors возвращаются обычным Service Hub response с `ok: false`.

Prefix `hub.` не используется Project Manager, потому что он зарезервирован Service Hub.

Project Manager v1 определяет:

- `project.invalid_request` — operation payload не соответствует contract;
- `project.unknown_operation` — operation отсутствует в Project Manager v1;
- `project.invalid_name` — имя не содержит non-whitespace символа;
- `project.name_too_long` — UTF-8 payload имени превышает внутренний лимит;
- `project.description_too_long` — UTF-8 payload описания превышает внутренний лимит;
- `project.not_found` — project ID отсутствует;
- `project.storage_error` — durable storage operation не выполнена;
- `project.id_generation_failed` — не удалось получить свободный generated ID;
- `project.internal_error` — непредвиденная Project Manager application failure.

Hub-generated ошибки (`hub.timeout`, `hub.cancelled`, `hub.provider_unavailable`, `hub.unknown_service` и другие) остаются частью Service Hub v1 и не переименовываются Project Manager.

## Cancellation

Service Hub может отправить provider message `cancel` для уже направленного request.

Текущие Project Manager CRUD operations короткие и выполняются синхронно. Step 3 принимает `cancel` как допустимое provider message, но не вводит отдельный acknowledgement и не обещает прерывание уже завершившейся SQLite операции.

Это соответствует Service Hub v1: provider-side cancellation является best-effort.

## Provider lifecycle and reconnect

Project Manager открывает provider WebSocket connection к Service Hub, запрашивает subprotocol `dispatcher.service-hub.v1` и первым application message отправляет:

```json
{
  "type": "register",
  "service": "project-manager.v1"
}
```

После `registered` provider обслуживает requests.

Если Service Hub connection закрывается:

- Project Manager process и SQLite storage продолжают жить;
- provider connection считается потерянным;
- provider выполняет повторные bounded retry attempts;
- после восстановления Service Hub создаётся новое WebSocket connection и повторяется `register`;
- Project Manager не сохраняет Service Hub route локально как authoritative state.

Shutdown `SIGINT`/`SIGTERM` останавливает reconnect loop и process завершается cleanly.

## Security boundary

`CORE-004` не реализует Users & Access.

Project Manager v1 payload не содержит временные поля:

- user ID;
- role;
- permissions;
- auth token;
- project ACL.

Эта модель добавляется в `CORE-005` согласованным способом поверх стабильной Project Manager/Service Hub boundary.
