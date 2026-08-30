# Внешний контракт Project Manager

## Статус

Контракт зафиксирован в `CORE-004 / Step 3`.

`CORE-005 / Step 5` добавляет backend-authoritative authorization поверх того же `project-manager.v1` business contract. Project payload/schema не получает `user_id`, role, permissions или session token.

Project Manager по-прежнему не вводит новый внешний транспорт и использует Service Hub v1.

## Transport boundary

Project Manager является provider Service Hub:

- Service Hub endpoint: `/v1/ws`;
- WebSocket subprotocol: `dispatcher.service-hub.v1`;
- provider service address: `project-manager.v1`.

После `CORE-005 / Step 4` защищённый client request передаёт session credential в общем transport field `auth`, отдельно от Project Manager business payload:

```json
{
  "type": "request",
  "id": "req-1",
  "service": "project-manager.v1",
  "operation": "list-projects",
  "payload": {},
  "auth": {
    "type": "session",
    "token": "64-lowercase-hex-characters"
  },
  "timeout_ms": 5000
}
```

Project Manager определяет только `operation`, `payload`, successful response payload, собственную authorization policy и provider-specific errors.

Service Hub проверяет transport shape `auth`, но не вычисляет identity/permissions. Project Manager не доверяет `user_id`, role или permissions из business payload.

Для authoritative access evaluation Project Manager открывает отдельное Service Hub **client** connection к `users-access.v1/evaluate-access`. Provider connection `project-manager.v1` остаётся provider-only. Между сервисами нет прямой C++ зависимости и нет доступа Project Manager к Users & Access SQLite.

## Versioning

Версия Project Manager contract зафиксирована service address:

`project-manager.v1`

Несовместимая будущая версия должна получить другой service address. Совместимые дополнения v1 не меняют семантику уже опубликованных operations.

Machine-readable payload definitions находятся в:

`services/project-manager/protocol/dispatcher/project_manager/v1/project_manager.schema.json`

`CORE-005 / Step 5` не меняет эту schema, потому что authentication остаётся transport context Service Hub, а authorization не добавляет business fields.

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

## Authorization policy

Все Project Manager v1 operations являются protected.

Subject определяется только через forwarded Service Hub session `auth`. Отсутствующая/недействительная session не получает project data.

Project Manager запрашивает актуальную access evaluation у `users-access.v1`; effective permissions не кэшируются в Project Manager. Поэтому disabled/expired session и изменение assignments отражаются на следующем authoritative request.

Capabilities остаются независимыми: `admin` не означает автоматически `view` или `edit`.

Policy Step 5:

- `create-project` требует `admin` в **global** scope;
- `list-projects` возвращает только проекты, где effective `view` разрешён; global `view` разрешает весь список;
- `get-project` требует `view` в scope конкретного project ID;
- `update-project` требует `edit` **или** `admin` в scope конкретного project ID.

Global assignments участвуют в project evaluation согласно Users & Access domain semantics. Таким образом, explicit global capability может удовлетворить соответствующую project-scoped проверку, но capability hierarchy не создаётся.

Если Users & Access unavailable, возвращает storage/crypto/internal failure либо authorization response невозможно надёжно разобрать, Project Manager работает fail-closed и не выполняет business operation.

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
- неизвестные поля не принимаются;
- caller должен иметь global `admin`.

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

Список фильтруется backend-side по effective `view`. Недоступные проекты не включаются в response.

Порядок списка в v1 не является отдельным контрактным обещанием сортировки.

### `get-project`

Request payload:

```json
{
  "id": "opaque-id"
}
```

`id` должен быть непустой строкой.

До чтения Project Manager требует effective `view` для указанного project scope. При отсутствии capability возвращается `access.forbidden` без project payload.

Successful payload совпадает с `create-project`.

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

До изменения Project Manager требует effective `edit` либо `admin` для указанного project scope.

Successful payload содержит обновлённый `project`.

Удаление проекта не входит в v1 baseline.

## Provider errors

Provider-specific errors возвращаются обычным Service Hub response с `ok: false`.

Prefix `hub.` зарезервирован Service Hub.

Project Manager v1 определяет:

- `project.invalid_request` — operation payload не соответствует contract;
- `project.unknown_operation` — operation отсутствует в Project Manager v1;
- `project.invalid_name` — имя не содержит non-whitespace символа;
- `project.name_too_long` — UTF-8 payload имени превышает внутренний лимит;
- `project.description_too_long` — UTF-8 payload описания превышает внутренний лимит;
- `project.not_found` — project ID отсутствует после успешной authorization;
- `project.storage_error` — durable Project Manager storage operation не выполнена;
- `project.id_generation_failed` — не удалось получить свободный generated ID;
- `project.authorization_unavailable` — authoritative Users & Access evaluation недоступна или не может быть надёжно завершена;
- `project.internal_error` — непредвиденная Project Manager application failure.

Security/session errors:

- `auth.invalid_session` — protected Project Manager request не имеет действующей session;
- `auth.session_expired` — Users & Access authoritative validation зафиксировала expiry;
- `access.forbidden` — session действительна, но необходимая capability отсутствует.

Hub-generated ошибки (`hub.timeout`, `hub.cancelled`, `hub.provider_unavailable`, `hub.unknown_service` и другие) остаются частью Service Hub v1. Ошибка внутреннего Project Manager → Users & Access request не прокидывается caller как `hub.*`: она превращается в `project.authorization_unavailable`, чтобы business service явно fail-closed на своей security dependency.

## Cancellation

Service Hub может отправить provider message `cancel` для уже направленного request.

Текущие Project Manager CRUD operations и access evaluations выполняются синхронно. Provider принимает `cancel` как допустимое message, но не вводит отдельный acknowledgement и не обещает прерывание уже завершившейся SQLite/access операции.

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

Authorization client использует отдельное client-role connection к тому же Service Hub. Connection создаётся лениво при первой access evaluation и переиспользуется последовательными Project Manager requests. Если Hub connection потеряна, authorization client сбрасывает её и один раз повторяет evaluation через новое connection.

Если Service Hub provider connection закрывается:

- Project Manager process и SQLite storage продолжают жить;
- provider connection считается потерянным;
- provider выполняет повторные bounded retry attempts;
- после восстановления Service Hub создаётся новое WebSocket connection и повторяется `register`;
- Project Manager не сохраняет Service Hub route локально как authoritative state.

Если `users-access.v1` временно не зарегистрирован, Project Manager process остаётся доступным как provider, но protected operations возвращают `project.authorization_unavailable` и не выполняются.

Shutdown `SIGINT`/`SIGTERM` останавливает reconnect loop и process завершается cleanly.

## Security boundary

Project Manager не является владельцем users/access records и не читает Users & Access storage.

Project Manager v1 payload не содержит:

- user ID;
- role;
- permissions;
- auth token;
- project ACL.

Authentication session передаётся только общим Service Hub `auth` field, а authorization вычисляется `users-access.v1/evaluate-access` по текущим durable assignments.

Web `ProjectContext` остаётся navigation/frontend context и не является доказательством доступа.
