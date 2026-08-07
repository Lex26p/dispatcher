# Текущее состояние проекта

**Дата состояния:** 7 августа 2026 года.  
**Репозиторий:** `https://github.com/Lex26p/dispatcher`.  
**Ветка:** `master`.  
**Последний подтверждённый SHA до этого пакета:** `7fc15e0b54348188fd9f3977c05532135d6026d3`.

## 1. Статус этапа

Этап общей горизонтальной продуктовой концепции завершён. Концепция прошла три независимые сквозные проверки; все принятые результаты встроены непосредственно в `PRODUCT_CONCEPT.md` и `PRODUCT_DECISIONS.md`.

Текущий реестр содержит `PRD-Q001–PRD-Q784`. Отдельные audit-отчёты, служебные журналы и дублирующие рабочие документы удалены из актуального дерева; история остаётся в Git.

## 2. Ключевые горизонтальные инварианты

- Одна core identity управляемого объекта; специализированные сервисы используют typed extensions, а не дубликаты объектов.
- Type, device profile и object template разделены.
- Configuration discovery/proposal отделено от authoritative runtime observation.
- Managed configuration, administrative transaction, package deployment и secret operation — разные классы изменений.
- Любой actuator action managed object проходит semantic Command Model; generic raw/network capability не даёт actuator authority.
- Publish, Deploy и Activate разделены; Edge имеет desired/active state и consistency domains.
- Каждый исполнительный контур имеет одного authoritative executor; потеря связи не означает потерю authority, а handover не выполняется только по timeout.
- Restore старого Full не вызывает автоматический downgrade более новой active configuration Edge; recovery divergence требует явного решения.
- Reconciliation распределённых runtime facts выполняется по классу сущности, без универсального last-write-wins.
- Observed source identity учитывает namespace, lifecycle guarantees и incarnation/reuse semantics.
- Operational exceptions имеют явный authoritative lifecycle и учитывают качество времени.
- Historian сохраняет provenance, time quality, gaps, counter segments и governed corrections без уничтожения исходных фактов.
- Notifications могут исполняться автономно на Edge по опубликованной policy со stable identities и deduplication после reconnect.
- Emergency Disable является persistent protective override и не отменяется обычным reconciler.
- Cross-domain sensitivity дополняет обычные permissions, не создавая отдельный DLP/IAM слой.
- Licensing не ослабляет safety, security, audit integrity и recoverability и не создаёт blind spots для реально observed объектов.
- Package removal отделяет runtime removal от retained historical semantic metadata.

## 3. Канонические файлы

1. `AGENTS.md` — правила работы.
2. `docs/product/PRODUCT_CONCEPT.md` — связная общая концепция, редакция 6.
3. `docs/product/PRODUCT_DECISIONS.md` — реестр решений через `PRD-Q784`.
4. `docs/product/DASHBOARDS_AND_MIMICS_CONCEPT.md` — углублённая тема дашбордов/мнемосхем.
5. `docs/project/OPEN_QUESTIONS.md` — индекс следующих предметных концепций.

## 4. Точка продолжения

Следующий этап — подробная продуктовая проработка предметных областей и специализированных сервисов. Конкретную тему выбирает пользователь; возможные направления перечислены в `OPEN_QUESTIONS.md`.

Эти концепции описывают зрелый продукт и могут уточнять горизонтальную модель. Это не порядок реализации.

## 5. Пока не начинаем

Без отдельного решения пользователя не формировать:

- roadmap и очередность реализации;
- программную архитектуру;
- технологический стек;
- схемы БД и внутренние API;
- исходный код.
