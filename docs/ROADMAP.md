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

### Результат Phase 2

Modbus-устройство и его точки можно полностью настроить через Web без ручного редактирования файлов.

## Phase 3 — SNMP

- [x] **S10A — SNMP runtime и persistent configuration**
  - новый `Dispatcher.Snmp`;
  - SNMP v2c GET;
  - polling OID;
  - преобразование SNMP values в общие runtime values;
  - Online/Offline через общий `DeviceStateService`;
  - SQLite schema v2;
  - миграция schema v1 → v2 без потери Modbus data;
  - persistent `snmp_devices` / `snmp_tags`;
  - `ConfigurationCatalog` содержит Modbus + SNMP;
  - глобальная уникальность `DeviceId` / `TagId`;
  - одновременный Modbus/SNMP runtime;
  - общий `RuntimeConfigurationCoordinator`;
  - protocol и hosted integration tests.

- [ ] **S10B — SNMP configuration API и Device Editor**
  - SNMP configuration DTO/contracts;
  - CRUD SNMP devices/tags;
  - protocol selection в Device Editor;
  - SNMP v2c properties;
  - OID editor;
  - live apply через общий coordinator;
  - одновременная ручная проверка Modbus + SNMP в Monitoring.

### Результат Phase 3

Modbus и SNMP одновременно опрашиваются, настраиваются через общий Device Editor и публикуют данные через единую runtime-модель тегов.

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
