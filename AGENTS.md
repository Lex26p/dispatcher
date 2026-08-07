# Инструкции для AI-ассистентов

**Область действия:** весь репозиторий.  
**Статус:** обязательные правила.  
**Дата актуализации:** 7 августа 2026 года.

Перед содержательной работой прочитать этот файл, затем `docs/project/PROJECT_STATE.md` и документы текущего этапа.

## 1. Источник истины

- Репозиторий: `https://github.com/Lex26p/dispatcher`.
- Рабочая ветка: `master`.
- Рабочая точка: последний полный SHA, подтверждённый пользователем.
- Репозиторий хранит долговременное состояние; чат используется для обсуждения.
- При противоречии между чатом и репозиторием приоритет имеет репозиторий, кроме нового прямого решения пользователя, которое ещё не зафиксировано.
- Проверять репозиторий соразмерно задаче; не тратить контекст на повторную исчерпывающую верификацию подтверждённого SHA без причины.

## 2. Рабочий процесс

Пользователь не редактирует содержательные файлы вручную. Ассистент формирует итоговые файлы, готовит ZIP от корня репозитория и перечисляет удаления отдельно. Пользователь распаковывает ZIP, выполняет минимальный Git-блок и присылает новый SHA.

Имя ZIP: `dispatcher-<area>-r<revision>.zip`. Внутри нет внешнего wrapper-каталога и `.git`.

Команда распаковки:

```powershell
Expand-Archive -Path "C:\Users\pereverworkki\Downloads\<archive>.zip" -DestinationPath "C:\Projects\dispatcher" -Force
```

Git:

```powershell
Set-Location "C:\Projects\dispatcher"
git status
git add -A
git commit -m "<message>"
git push
git rev-parse HEAD
```

Не добавлять без необходимости `git diff`, повторную проверку SHA, branches или pull requests.

## 3. Уровни спецификации

### Product Concept

Отвечает: **что такое Dispatcher и какие фундаментальные продуктовые инварианты обязательны**.

Канон: `docs/product/PRODUCT_CONCEPT.md` и `docs/product/PRODUCT_DECISIONS.md`.

Общую концепцию не расширять по инерции. Возвращаться к ней только если functional/domain work выявил реальное фундаментальное противоречие или недостающий горизонтальный принцип.

### Functional Specification

Отвечает: **как продукт должен работать для пользователя и как наблюдаются его функциональные состояния/переходы**.

Канон текущего этапа: `docs/functional/`.

Допустимы роли, user flows, registries/editors, lifecycle, actions, validations, errors, permissions, Full/Compact/Edge observable behaviour, realtime UI semantics и acceptance-level requirements.

### System Architecture / Technical Specification

Отвечает: **как система технически обеспечивает принятые требования**.

До architecture-readiness gate не фиксировать без отдельной причины frameworks, БД, внутренние protocols, message brokers, storage schemas, consensus/fencing implementation и прочие технические решения.

## 4. Правила продуктовой и функциональной работы

- Все документы описывают зрелый целевой продукт, а не MVP.
- Roadmap функционального этапа описывает порядок **проработки**, а не порядок появления функций в зрелом продукте.
- Не откладывать функциональность словами «позже» только потому, что её документ будет прорабатываться следующим.
- Продуктовый сервис не равен программному микросервису.
- Отдельная инженерная область не становится отдельным сервисом без собственной устойчивой предметной модели и workflow.
- Крупные темы обсуждаются небольшими раундами вопросов с вариантами, последствиями и рекомендацией.
- Если пользователь не возражает и переходит дальше, рекомендованный вариант считается принятым.
- Прямое пользовательское уточнение заменяет прежнюю формулировку, а не создаёт параллельное решение.
- Для functional specs использовать namespace области: `ENG-Q/ENG-FR`, `OPS-Q/OPS-FR`, `WEB-Q/WEB-FR` и т. д.
- После принятия вопроса его устойчивый смысл сводится в functional requirements соответствующего файла; отдельный общий журнал functional decisions не создавать.
- `docs/functional/ROADMAP.md` — временный рабочий файл и в конце функционального этапа удаляется; Git хранит историю.
- Сквозную согласованность проверять через `docs/functional/REFERENCE_SCENARIOS.md`.

## 5. Канонические документы

### Общая концепция

- `docs/product/PRODUCT_CONCEPT.md`
- `docs/product/PRODUCT_DECISIONS.md`
- `docs/product/DASHBOARDS_AND_MIMICS_CONCEPT.md`

### Текущее состояние

- `docs/project/PROJECT_STATE.md`
- `docs/project/OPEN_QUESTIONS.md`

### Функциональная спецификация

- `docs/functional/README.md`
- `docs/functional/ROADMAP.md` — временный навигатор этапа
- `docs/functional/REFERENCE_SCENARIOS.md`
- `docs/functional/engineering/ENGINEERING_CONFIGURATION.md`
- `docs/functional/operations/OPERATIONS.md`
- `docs/functional/web-platform/WEB_PLATFORM.md`

Новые functional-файлы создавать только при фактическом начале области. Не создавать каталоги-заглушки под весь продукт.

## 6. Текущий этап

Общая продуктовая концепция завершена решениями `PRD-Q001–PRD-Q803`. Текущий этап — Functional Specification.

Ближайшая содержательная работа:

1. Engineering / Configuration (`ENG-*`);
2. Operations / Dispatcher Workspace (`OPS-*`);
3. Web Platform (`WEB-*`);
4. Architecture Readiness Review #1.

Первый architecture-readiness gate не требует заранее закончить функциональную детализацию всех VMS/ТОиР/ACS/vertical областей. Он требует устойчивого центрального контура Engineering → runtime/Edge → Operations → command/result/audit и общего Web contract.
