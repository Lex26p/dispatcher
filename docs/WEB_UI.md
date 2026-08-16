# Web UI — правила интерфейса Dispatcher

Этот документ задаёт базовые правила компоновки и поведения Web-интерфейса Dispatcher.

Цель — сохранить единый инженерный стиль при развитии мониторинга, редактора устройств, SNMP-инструментов и редактора мнемосхем.

## 1. Главный принцип

Максимум доступной высоты и ширины экрана отдаётся рабочей области.

Dispatcher — рабочий инженерный инструмент, а не публичный информационный сайт. Поэтому не следует расходовать значительную часть экрана на:

- большие заголовки страниц;
- декоративные hero-блоки;
- повторяющиеся breadcrumbs;
- большие вертикальные отступы;
- крупные карточки там, где эффективнее список;
- несколько постоянно видимых уровней навигации без необходимости.

Минимализм не должен скрывать важные рабочие данные.

## 2. Базовая структура приложения

```text
┌─────────────────────────────────────────────────────────────────────┐
│ ☰  Dispatcher                                     status / actions │
├──────────────┬──────────────────────────────────────┬───────────────┤
│              │ contextual toolbar, если нужен       │               │
│ Локальная    ├──────────────────────────────────────┤ Свойства      │
│ навигация    │                                      │ выбранного    │
│              │                                      │ объекта       │
│              │          Рабочая область             │               │
│              │                                      │               │
│              │                                      │               │
└──────────────┴──────────────────────────────────────┴───────────────┘
```

Не каждый экран обязан использовать все три панели. Но редакторы по умолчанию строятся именно по этой схеме.

## 3. Глобальный header

В верхней части приложения находится один компактный глобальный header.

Требования:

- занимает только необходимую высоту;
- слева находится кнопка `☰` в authenticated application shell;
- рядом допускается название приложения/текущего глобального контекста;
- справа могут размещаться только действительно глобальные состояния и действия;
- для authenticated user справа показываются current user и compact logout action;
- header не должен превращаться в крупную декоративную шапку.

Не фиксируется конкретная высота в пикселях до появления реального UI. Главный критерий — header не должен отнимать заметную часть рабочей высоты.

Anonymous login state является специальным глобальным состоянием: он сохраняет compact header, но не показывает `☰`, потому что service navigation до входа не является доступным workflow.

## 4. Глобальная навигация

Кнопка `☰` открывает глобальную навигацию между основными сервисами/разделами приложения.

Пример будущей структуры:

```text
Monitoring
Device Editor
Mimic
...
```

Глобальная навигация:

- доступна в authenticated application shell;
- по умолчанию скрыта;
- открывается по `☰`;
- не должна постоянно отнимать ширину рабочей области;
- используется для перехода между крупными сервисами, а не для навигации внутри одного редактора;
- в anonymous login state не показывается.

Если в будущем появится необходимость закреплять глобальную навигацию, это рассматривается отдельным решением.

## 5. Локальная навигация

После входа в конкретный сервис слева постоянно располагается его локальная навигация.

Примеры:

### Device Editor

```text
Devices
├── PLC-01
├── PLC-02
├── Switch-01
└── ...
```

### Mimic Editor

```text
Mimics
├── Boiler room
├── Pump station
└── ...
```

Локальная навигация отвечает на вопрос:

> С каким объектом/документом внутри текущего сервиса мы сейчас работаем?

Она не заменяет глобальную навигацию.

## 6. Центральная рабочая область

Центр экрана — основная рабочая область.

Она должна получать максимальную долю доступного пространства.

Примеры:

- таблица текущих значений;
- конфигурация устройства;
- список/редактор тегов;
- canvas мнемосхемы;
- диагностический экран.

Не следует помещать основной инструмент в маленькую карточку по центру большого пустого пространства.

## 7. Панель свойств

Справа располагается панель свойств выбранного объекта.

Общий паттерн:

```text
выбор объекта
     ↓
свойства объекта справа
```

Примеры:

- выбрано устройство → свойства устройства;
- выбран тег → свойства тега;
- выбран элемент мнемосхемы → координаты, размеры, binding и визуальные свойства.

Положение панели свойств должно быть консистентным между редакторами.

Если объект не выбран, панель может быть пустой, показывать подсказку или при необходимости сворачиваться. Поведение конкретного сервиса определяется при его реализации.

## 8. Контекстный toolbar / второй header

Над рабочей областью допускается второй компактный toolbar, если он действительно нужен текущему сервису.

Примеры:

### Device Editor

```text
PLC-01                    Online     Save     Test
```

### Mimic Editor

```text
Select | Text | Rectangle | Indicator | Zoom
```

Toolbar не добавляется только ради визуального разделения.

Если для текущей страницы нет полезных контекстных действий, дополнительная строка не нужна.

## 9. Списки вместо крупных карточек

Устройства и другие инженерные сущности по умолчанию отображаются плотным списком или таблицей.

Предпочтительно:

```text
Name          Protocol      Address          Status
---------------------------------------------------
PLC-01        Modbus TCP    192.168.1.10     Online
PLC-02        Modbus TCP    192.168.1.11     Online
PLC-03        Modbus TCP    192.168.1.12     Offline
Switch-01     SNMP          192.168.1.20     Online
```

Не предпочтительно без отдельной причины:

```text
┌────────────────────┐
│                    │
│       PLC-01       │
│       ONLINE       │
│                    │
└────────────────────┘
```

Карточки допустимы, если они реально лучше решают задачу, но не являются стилем по умолчанию.

## 10. Информационная плотность

Интерфейс должен позволять оператору или инженеру видеть достаточно данных без постоянного скроллинга и переходов.

Предпочитаются:

- компактные строки;
- выровненные колонки;
- таблицы;
- короткие подписи;
- группировка по смыслу;
- постоянное положение ключевых панелей.

Следует избегать чрезмерно больших:

- padding;
- margin;
- controls;
- typography;
- empty states.

При этом плотность не должна ухудшать читаемость и точность управления.

## 11. Состояния системы видны сразу

Operational-состояния не скрываются ради чистоты дизайна.

Особенно важны:

- `Online`;
- `Offline`;
- connection problems;
- quality;
- stale/устаревшие данные;
- состояние выполняемой команды, когда это потребуется.

Если данные больше нельзя считать актуальными, UI должен это показывать, а не продолжать отображать последнее значение как нормальное текущее состояние.

## 12. Консистентность редакторов

Разные редакторы должны использовать одинаковую пространственную модель, когда это возможно:

```text
слева  → выбор/структура
центр  → работа
справа → свойства
сверху → только необходимые actions
```

Это позволяет пользователю один раз освоить модель взаимодействия и применять её во всех сервисах.

## 13. Responsive-поведение

Основная целевая среда — desktop/рабочая станция.

Responsive layout не должен ломать desktop-компоновку ради мобильного режима.

На узком экране допустимо:

- сворачивать панель свойств;
- сворачивать локальную навигацию;
- показывать панели по запросу.

Точные breakpoint-правила определяются после появления реального application shell.

## 14. UI-библиотеки и компоненты

Выбор UI-библиотеки сам по себе не должен диктовать архитектуру экрана.

Если готовый компонент предлагает:

- большой page title;
- большие карточки;
- лишний navigation rail;
- чрезмерные отступы,

его следует адаптировать под правила Dispatcher, а не менять правила Dispatcher под defaults библиотеки.

## 15. Правило для будущих изменений

Перед реализацией нового Web-экрана агент должен ответить на четыре вопроса:

1. Что является глобальной навигацией?
2. Что является локальной навигацией текущего сервиса?
3. Что является основной рабочей областью?
4. Какие свойства выбранного объекта должны находиться справа?

Если экран не является редактором и какая-то панель ему не нужна, её не следует добавлять искусственно.

Отступление от этого документа допустимо только при явной функциональной причине или новом решении пользователя.

## 16. Authentication UI

Authentication является глобальным состоянием application shell, а не отдельным инженерным editor service.

Anonymous state использует минимальную компоновку:

```text
┌───────────────────────────────────────────────────────────┐
│ Dispatcher   Вход                                        │
├───────────────────────────────────────────────────────────┤
│                                                           │
│             Имя пользователя  [___________]               │
│             Пароль            [___________]               │
│                                  [ Войти ]                 │
│                                                           │
└───────────────────────────────────────────────────────────┘
```

Правила anonymous state:

- глобальный header остаётся compact;
- `☰` и service drawer не показываются;
- service workspace не рендерится за login form;
- login form занимает только необходимую ширину и не превращается в крупный marketing/hero screen;
- искусственные left navigation/right properties panels не добавляются;
- authentication/API error показывается рядом с login flow и остаётся конкретным наблюдаемым состоянием.

Authenticated state использует обычный application shell. Справа в global header показываются:

```text
DisplayName
UserName
Выйти
```

Имя пользователя является глобальной identity state, поэтому оно находится в global header, а не дублируется в каждом service toolbar.

Login/logout не должны принудительно сбрасывать текущий service route. Если пользователь открыл `/events`, `/history` или другой route до входа, successful login возвращает его в этот же context; после logout URL также может сохраняться для повторного входа.

Скрытие navigation/workspace для anonymous user является UX behavior, а не security mechanism. Web visibility/enabled state может отражать permissions только после появления соответствующей Server authorization boundary; client-side hiding никогда не заменяет Server enforcement.

## 17. Permission-aware visibility и enabled state

Начиная с V2-S08C Web отражает effective permissions, которые Server возвращает для текущего authenticated user.

Это правило относится только к UX:

```text
Web visibility / disabled state
        ≠
Server authorization
```

Окончательная защита REST/SignalR всегда выполняется Server-side.

Текущая service visibility:

```text
Мониторинг        → Runtime.Read
Мнемосхемы        → Runtime.Read
История / Тренды  → Runtime.Read
События           → Runtime.Read
Редактор устройств → Runtime.Read + Devices.Edit
Редактор мнемосхем → Runtime.Read + Mimics.Edit
Тревоги            → Runtime.Read + Alarms.Configure
```

Dedicated editor service без edit permission не должен показываться как доступный workflow. Если пользователь вручную открывает такой URL, обычный global header сохраняется, но editor workspace не рендерится; вместо него показывается компактное состояние «Недостаточно прав» с требуемыми permission identifiers.

Mutation controls внутри доступного runtime service отражают permission отдельно от технической writable capability:

```text
configured tag Writable + Tags.Write
    → input / Записать

configured tag Writable + no Tags.Write
    → read-only marker

Mimic Button + Tags.Write + writable TagId
    → enabled command

Mimic Button + no Tags.Write
    → visible but disabled
```

Mimic Button остаётся видимым, потому что является частью operational схемы; скрывать элемент целиком означало бы искажать саму мнемосхему. Отключается только command interaction.

Если authenticated user не имеет `Runtime.Read`, global header с identity/logout остаётся доступным, а drawer сообщает, что доступных текущих сервисов нет. Это допускает будущие admin-only/custom roles без фиктивного доступа к runtime.

Web не должен проверять role names (`Viewer`, `Engineer`, `Administrator`) для visibility. Используются только stable permission identifiers, совпадающие с Server authorization model.

## 18. Users / Roles administration

V2-S09B добавляет administrative service:

```text
/security
Пользователи / Роли
```

Он появляется в global navigation, если current user имеет хотя бы одну capability:

```text
Users.Manage OR Roles.Manage
```

`Runtime.Read` для этого service не требуется: admin-only custom role является допустимым состоянием. Client route visibility остаётся UX projection; Server S09A авторизует каждый API endpoint отдельно.

Spatial model:

```text
слева  → local navigation Пользователи / Роли
центр  → compact toolbar + dense users/roles table
справа → selected user/role properties и effective permissions
```

Не используются большие карточки или отдельный декоративный page header. Status/error показываются в compact toolbar/workspace boundary.

User section:

- доступен при `Users.Manage`;
- таблица показывает login, display name, Enabled/Disabled, количество roles и effective permissions;
- справа находятся immutable login/UserId, editable `DisplayName`/`Enabled`, assignments, effective permissions и password reset;
- create user выполняется в той же right properties panel;
- role assignments и password reset доступны только когда одновременно есть `Users.Manage + Roles.Manage`;
- без `Roles.Manage` assigned role IDs остаются read-only information.

Roles section:

- доступен при `Roles.Manage`;
- таблица показывает name, Built-in/Custom, assignment count и permission count;
- built-in role всегда read-only по Server-projected `BuiltIn`;
- custom role редактирует name и declared permission set справа;
- delete custom role disabled в Web при existing assignments, но окончательный conflict определяет Server.

Permission-aware UI не должен вызывать недоступную collection API только для оформления screen:

```text
Users.Manage only → no GET /api/security/roles
Roles.Manage only → no GET /api/security/users
```

После successful mutation current authentication projection refresh-ится с Server, поэтому изменение current actor metadata/permissions сразу меняет global header/navigation. Role names не используются для visibility/enabled state.

## 19. Alarm Editor

V2-S10B добавляет service:

```text
/alarms
Тревоги
```

Global navigation показывает его только при:

```text
Runtime.Read + Alarms.Configure
```

Spatial model строго следует общему editor contract:

```text
слева  → alarm definitions
центр  → compact toolbar + dense rules table
справа → selected definition properties
сверху → Add / Save / Delete / Refresh
```

Left panel показывает `Name`, `AlarmId`, Enabled state и severity компактно; крупные карточки не используются. Center table позволяет сравнивать `AlarmId`, `Name`, `TagId`, condition, severity и delay без перехода между отдельными страницами.

Right properties содержит:

```text
AlarmId
Name
Enabled
TagId
Condition
Threshold / Hysteresis when applicable
Severity
Delay
Message
```

Persisted `AlarmId` read-only. Новый definition редактирует ID до первого Save. `TagId` выбирается из current Modbus/SNMP logical tags. Stale persisted binding не скрывается: selector показывает старый ID и explicit warning; Save остаётся недоступным, пока не выбран current tag.

`DigitalTrue/DigitalFalse` не показывают numeric fields. `High/Low` показывают Threshold и Hysteresis. Это только configuration UX: экран не отображает Active/Acknowledged state и не пытается применять delay/hysteresis самостоятельно до V2-S11/V2-S12.

Dirty draft обозначается compact `несохранено`. Selection change и Refresh при dirty state требуют подтверждения discard. Server validation/error (`ProblemDetails.detail`) показывается в рабочей области рядом с toolbar.

Client route visibility не является security mechanism; S10A Server authorization остаётся окончательной authority.
