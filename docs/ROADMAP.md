# Дорожная карта Dispatcher

Дорожная карта изменяется по мере развития проекта. Количество шагов не является жёстким: шаг можно разделить или объединить, если это уменьшает риск и сохраняет проверяемый результат.

## Обозначения

- `[x]` — содержимое шага подготовлено/реализовано в репозитории.
- `[ ]` — шаг ещё не реализован.
- Финальное принятие каждого технического шага подтверждается пользователем после локальной проверки и отправки нового Git SHA.
- Ошибка проверки означает продолжение текущего шага, а не переход к следующему.

## Phase 0 — Основа проекта

- [x] **S00 — Документация проекта**

## Phase 1 — Первый вертикальный срез: Modbus → Web

- [x] **S01 — Минимальный .NET solution**
- [x] **S02 — Core и TagService**
- [x] **S03 — Минимальный Modbus TCP read**
- [x] **S04 — Polling и состояние устройства**
- [x] **S05 — ASP.NET Core API**
- [x] **S06 — Blazor WebAssembly read UI**
- [x] **S07A — Hosted Modbus polling**
- [x] **S07B — Write path Web → Modbus**

## Phase 2 — Редактор устройств

- [x] **S08 — Постоянная конфигурация**
- [x] **S09A — Configuration CRUD API и live apply**
- [x] **S09B — Blazor Device Editor**

## Phase 3 — SNMP

- [x] **S10A — SNMP runtime и persistent configuration**
  - `Dispatcher.Snmp`;
  - SNMP v2c GET;
  - polling OID;
  - общий `TagService` / `DeviceStateService`;
  - SQLite schema v2 и migration;
  - simultaneous Modbus/SNMP runtime;
  - `RuntimeConfigurationCoordinator`.

- [x] **S10B — SNMP configuration API и Device Editor**
  - SNMP configuration DTO/contracts;
  - CRUD SNMP devices/tags;
  - единый mutation lock для Modbus/SNMP configuration;
  - protocol selection при создании device;
  - SNMP v2c Host/Port/Community properties;
  - OID editor;
  - explicit Save;
  - live apply через общий coordinator;
  - единое дерево Modbus/SNMP devices;
  - protocol badges;
  - integration tests CRUD, validation, cross-protocol ID и live apply.

### Результат Phase 3

Modbus TCP и SNMP v2c одновременно опрашиваются, полностью настраиваются через общий Device Editor и публикуют данные через единую runtime-модель тегов.

## Phase 4 — Простая мнемосхема

- [ ] **S11 — Runtime мнемосхемы**
  - Text;
  - Rectangle;
  - Value;
  - Indicator;
  - Button;
  - binding по `TagId`;
  - realtime;
  - простое управление.

- [ ] **S12 — Минимальный редактор мнемосхемы**
  - создание схемы;
  - добавление/удаление элементов;
  - позиция и размер;
  - свойства;
  - выбор тега;
  - сохранение и загрузка;
  - общий editor layout.

## Phase 5 — Дальнейшее развитие

Не детализируется заранее.

После S12 при необходимости планируются historian, alarms/events, пользователи/роли, дополнительные протоколы, шаблоны, scripting, redundancy и distributed execution.
