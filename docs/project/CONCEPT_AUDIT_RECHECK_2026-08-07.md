# Повторная независимая проверка и закрытие остаточных замечаний

**Дата:** 7 августа 2026 года.  
**Проверенный commit:** `339a706ca988679f7d429cd0976d786e9378602b`.  
**Статус:** завершена; остаточные замечания разобраны и закрыты продуктовыми решениями.

## 1. Итог независимой повторной проверки

Аудитор проверял качество закрытия `AUD-001–AUD-026` решениями `PRD-Q528–PRD-Q676`. Результат до дополнительного разбора:

- `CLOSED` — 24;
- `PARTIALLY CLOSED` — 1 (`AUD-009`);
- `STILL OPEN` — 0;
- `SUPERSEDED` — 1 (`AUD-022`);
- новых P0 — 0;
- новых P1 — 1 (`AUD2-001`);
- новых P2 — 1 (`AUD2-002`).

`AUD-009` был содержательно решён observed-object моделью, но старый `PRD-Q042` смешивал configuration discovery с authoritative runtime observation. `AUD2-001` выявил риск автоматического снятия Emergency Disable обычным desired-state reconciliation. `AUD2-002` выявил неясность лицензирования автоматически возникающих observed objects сверх object-scale entitlement.

## 2. Закрытие AUD-009

`PRD-Q042R` заменяет старую широкую формулировку:

- configuration discovery/proposal предлагает потенциальную новую управляемую конфигурацию и идёт через review/editor draft/publish;
- authoritative runtime observation создаёт или обновляет observed object как эксплуатационный факт без индивидуальной публикации;
- import остаётся отдельным строгим способом ввода только новой конфигурации в editor draft.

Совместно с `PRD-Q584–PRD-Q591` это переводит `AUD-009` в `CLOSED`.

## 3. Закрытие AUD2-001 — Emergency Disable и reconciliation

Решения `PRD-Q677–PRD-Q682` фиксируют:

- Emergency Disable имеет приоритет над ordinary reconciliation;
- после выполнения это persistent protective override над desired configuration;
- override переживает restart/HA/service restart;
- обычный reconciler не имеет права автоматически re-enable отключённую функцию;
- снятие выполняется только отдельным явно авторизованным действием или осознанным урегулированием опубликованной конфигурации с явным release;
- emergency protective operation направлено только к более безопасному/ограниченному состоянию;
- UI показывает desired, actual, active override и причину divergence.

`AUD2-001` закрыт.

## 4. Закрытие AUD2-002 — dynamic observed objects и licensing

Решения `PRD-Q683–PRD-Q690` фиксируют:

- реальный observed resource не становится невидимым из-за license exceed;
- сохраняется mandatory visibility baseline: identity, presence, source/type, основные состояния, connectivity, critical alarms/events и diagnostics;
- расширенные historian/analytics/automation/management capabilities могут регулироваться прозрачной license policy;
- известное критическое состояние не скрывается коммерческим ограничением;
- safety/emergency action существующего наблюдаемого контура не блокируется лицензией;
- dynamic/ephemeral licensing metric должна быть явной и учитывать специфику таких объектов;
- exceed не выключает уже работающий visibility baseline;
- mandatory safety visibility не означает бесплатную полную management functionality.

`AUD2-002` закрыт.

## 5. Итог после решений

После `PRD-Q042R` и `PRD-Q677–PRD-Q690` первый аудит и повторная проверка качества исправлений считаются закрытыми на уровне общей продуктовой концепции. Это не означает, что концепция окончательно доказана: третья независимая проверка выполняется в новом чате с чистого листа, чтобы не ограничивать поиск уже известными AUD.
