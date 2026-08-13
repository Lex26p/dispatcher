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

### Результат Phase 1

Рабочая система подключается к настроенному Modbus TCP устройству, читает `UInt16` Holding Registers, показывает их в Web в реальном времени и записывает явно разрешённые значения обратно.

## Phase 2 — Редактор устройств

- [x] **S08 — Постоянная конфигурация**
  - модели Modbus devices/tags;
  - SQLite;
  - schema version;
  - загрузка configuration при старте;
  - in-memory `ConfigurationCatalog`;
  - persistent configuration отделена от runtime state.

- [x] **S09A — Configuration CRUD API и live apply**
  - REST snapshot Modbus configuration;
  - создать/редактировать/удалить устройство;
  - создать/редактировать/удалить тег;
  - server-side validation и duplicate checks;
  - сохранение snapshot в SQLite;
  - замена `ConfigurationCatalog`;
  - остановка и перезапуск polling loops без перезапуска Server;
  - сброс устаревшего runtime state;
  - SignalR `ConfigurationChanged`;
  - integration tests persistence/live apply.

- [x] **S09B — Blazor Device Editor**
  - глобальная навигация `Редактор устройств`;
  - локальное дерево устройств и тегов;
  - создать/редактировать/удалить через S09A API;
  - центральная таблица тегов выбранного устройства;
  - свойства выбранного объекта справа;
  - компактный toolbar;
  - explicit Save поверх client-side draft;
  - отображение validation/server errors;
  - предупреждение о несохранённых изменениях;
  - ручная проверка полного сценария через Web.

### Результат Phase 2

Modbus-устройство и его точки можно полностью настроить через Web без ручного редактирования файлов.

## Phase 3 — SNMP

- [ ] **S10 — SNMP как второй протокол**
  - SNMP-компонент;
  - polling OID;
  - преобразование SNMP-данных в общие теги;
  - интеграция SNMP-настроек в редактор устройств;
  - одновременная работа Modbus и SNMP.

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
