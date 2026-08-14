# Dispatcher

`Dispatcher` — развиваемая система диспетчеризации для опроса, управления и визуализации устройств через разные промышленные и сетевые протоколы.

Phase 1 Modbus → Web и Phase 2 Device Editor завершены. Phase 3 добавляет второй протокол — SNMP — без изменения общей runtime-модели логических тегов.

## Рабочая цепочка

```text
                 ┌── Modbus TCP
                 │
Protocol workers ├── SNMP v2c
                 │
                 ↓
      TagService / DeviceStateService
                 ↓
         ASP.NET Core Server
                 ↓
           REST + SignalR
                 ↓
        Blazor WebAssembly
```

После S10A система умеет:

1. Читать Modbus Holding Register `UInt16` через FC03.
2. Записывать разрешённые Modbus Holding Register через FC06.
3. Хранить Modbus device/tag configuration в SQLite.
4. Редактировать Modbus configuration через Web.
5. Опросить SNMP v2c OID через GET.
6. Преобразовать SNMP varbind в общий `TagService`.
7. Использовать общий `DeviceStateService` для Modbus и SNMP.
8. Загружать Modbus и SNMP configuration из одной SQLite database.
9. Одновременно запускать Modbus и SNMP polling workers.

SNMP configuration API и поля SNMP в Device Editor добавляются в S10B.

## Базовый стек

- Backend/Core: C# / .NET 10.
- Server API: ASP.NET Core.
- Web: Blazor WebAssembly.
- Realtime: SignalR.
- SQLite provider: Microsoft.Data.Sqlite 10.0.10.
- SQLite native bundle: SQLitePCLRaw.bundle_e_sqlite3 2.1.12.
- Modbus: NModbus 3.0.83.
- SNMP: Lextm.SharpSnmpLib 12.5.7.
- SNMP scope S10A: v2c GET.

## Runtime

```text
ModbusRuntimeHostedService ─┐
                            ├─→ TagService
SnmpRuntimeHostedService ───┘       ↓
                              REST / SignalR
```

`TagService` по-прежнему хранит только:

```text
TagId
Value
Timestamp
```

`DeviceStateService` по-прежнему хранит protocol-neutral Online/Offline.

Для SNMP current value конвертируются распространённые типы:

```text
Integer32   → Int32
Counter32   → UInt32
Gauge32     → UInt32
TimeTicks   → UInt32
Counter64   → UInt64
OctetString → String
```

Остальные поддержанные library-типы временно публикуются через строковое представление.

## SQLite schema

Schema version после S10A:

```text
2
```

Таблицы:

```text
modbus_devices
modbus_tags

snmp_devices
snmp_tags
```

При открытии существующей schema version `1` Server автоматически добавляет SNMP tables и переводит БД в version `2`, не удаляя Modbus records.

SNMP device configuration:

```text
DeviceId
Name
Enabled
Host
Port
Community
PollIntervalMilliseconds
RequestTimeoutMilliseconds
```

SNMP tag:

```text
TagId
Name
Oid
```

S10A поддерживает SNMP v2c, поэтому `Community` хранится как часть configuration. SNMP v3 пока не реализован.

## Общая уникальность ID

`DeviceId` и `TagId` должны быть уникальны между всеми protocol configurations:

```text
Modbus + SNMP
```

Это необходимо, потому что runtime services индексируют current state общими logical identifiers.

## Live apply

После S10A отдельный protocol worker больше не очищает глобальное runtime state самостоятельно.

Общий алгоритм configuration apply:

```text
stop Modbus polling
stop SNMP polling
        ↓
clear TagService / DeviceStateService один раз
        ↓
start Modbus polling
start SNMP polling
```

За это отвечает `RuntimeConfigurationCoordinator`.

Текущий Modbus Device Editor продолжает использовать уже существующий CRUD API. Его Save теперь перезапускает оба protocol runtime из единого active configuration snapshot, поэтому SNMP runtime не теряется после изменения Modbus configuration.

## SNMP protocol boundary

`Dispatcher.Snmp` зависит от:

```text
Dispatcher.Core
Lextm.SharpSnmpLib
```

и не зависит от Server/Web/Modbus.

Один SNMP poll-cycle отправляет GET с настроенным набором OID. Результаты публикуются в `TagService` после успешного ответа. Ошибка/timeout переводит устройство в Offline.

## Web

Monitoring не требует изменений для SNMP:

```text
SNMP value → TagService → REST/SignalR → Monitoring
```

SNMP tags на S10A read-only; `Writable = false`.

S10B расширит существующий `/devices` editor:

```text
Protocol = Modbus TCP | SNMP v2c
```

с protocol-specific properties справа.

## Следующий шаг

**S10B — SNMP configuration API + Device Editor integration.**

После него пользователь сможет создать SNMP device и OID tags через Web и одновременно видеть Modbus/SNMP данные в Monitoring.

## Документы

- [Архитектура](docs/ARCHITECTURE.md)
- [Дорожная карта](docs/ROADMAP.md)
- [Архитектурные решения](docs/DECISIONS.md)
- [Правила Web UI](docs/WEB_UI.md)
- [Правила для AI-агентов](AGENTS.md)
