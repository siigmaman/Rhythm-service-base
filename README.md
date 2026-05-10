# rhythm-service

Сервис на **C++20** и **[userver](https://github.com/userver-framework/userver)** с тремя HTTP-методами: приём события (`POST /action`), выдача рекомендаций (`GET /recommend`), карточка товара (`GET /product`). Задуман как **учебный проект**: разобраться с асинхронным фреймворком, вынести бизнес-логику отдельно от транспорта, опционально стыковать Postgres, Redis и RabbitMQ без переписывания хендлеров. Продакшен-обвязка (полноценный LLM вместо заглушки GPT, единый auth, мониторинг) сюда намеренно не затаскивалась — только то, что нужно, чтобы код собирался, запускался и был понятен по структуре.

Секреты и строки подключения — через **Secdist** (`configs/secdist_*.example.json`); для локального прогона удобен `docker compose`.

## Архитектура

### Слои и каталоги

Код разнесён по четырём зонам ответственности:

1. **`handlers/`** — HTTP: разбор запроса, код ответа, JSON. Классы в `rhythm::handlers`, разбор JSON — `rhythm::handlers::http_json`. Зависимости от бэкендов только через `app/port_resolution.hpp` и `ComponentContext` userver.

2. **`services/`** — сценарии без знания о HTTP: `ActionService` (валидация, идемпотентность, публикация в очередь), `RecommendationService` (сбор источников, взвешенный merge, кэш ленты, учёт визитов), `ProductService` (чтение каталога и сборка JSON ответа).

3. **`domain/`** — правила TTL ленты и классификации визита (`ttl_policy`), без привязки к Redis или памяти.

4. **`infra/`** — контракты и реализации:
   - **`ports.hpp`** — интерфейсы: кэш ленты, side-cache (популярное / GPT / похожие), очередь событий, каталог, «GPT», хранилище поведения пользователя, идемпотентность.
   - **`in_memory_stubs.*`** — процессные заглушки и `InMemoryRhythmData` (лента, side-слои, дедуп в одном mutex).
   - **`pg_rhythm_data.*`**, **`redis_rhythm_data.*`**, **`rabbitmq_event_publisher.*`** — реальные бэкенды при соответствующих флагах CMake.
   - **`visit_ledger.*`** — журнал последнего визита для политики ленты; при Redis его роль выполняет `RedisRhythmData`.
   - **`redis_keyspace.hpp`** — соглашение об именах ключей и hash-tag `{...}` под Redis Cluster.

5. **`app/`** — точка склейки с userver:
   - **`port_resolution.cpp`** — единственное место, где ветвление по **`RHYTHM_WITH_POSTGRES`**, **`RHYTHM_WITH_REDIS`**, **`RHYTHM_WITH_RABBITMQ`**: либо `FindComponent` на зарегистрированный компонент, либо ссылка на **`GlobalBeans()`** (статические заглушки в памяти).
   - **`component_bootstrap.cpp`** — регистрация в `ComponentList` опциональных компонентов userver (Postgres, Redis, Rabbit) и обёрток `*RhythmData` / publisher.
   - **`beans.hpp`** — один набор заглушек на процесс для минимальной сборки.

6. **`workers/`** — отдельный исполняемый файл: consumer RabbitMQ, запись в `user_actions`, при необходимости инвалидация ключей в Redis.

Поток данных не рисуется отдельной диаграммой: связи читаются по цепочке handler → `port_resolution` → сервис → интерфейс из `ports.hpp` → конкретный класс в `infra/`.

### Контракты и кто их реализует

| Порт | Назначение | Без флагов CMake | С флагами |
|------|------------|------------------|-----------|
| `IEventQueuePort` | Публикация JSON в брокер | `LoggingEventQueue` (лог) | `RabbitMqEventPublisher` |
| `IIdempotencyPort` | Резерв ключа `Idempotency-Key` | `InMemoryRhythmData` | `RedisRhythmData` |
| `IRecommendationCachePort` | Лента рекомендаций на пользователя | то же in-memory | Redis |
| `ISideRecommendationCache` | Кэши популярного, GPT, похожих | то же | Redis |
| `IVisitLedger` | Время последнего `GET /recommend` | `VisitLedger` | Redis |
| `IGptRecommendPort` | Ранжирование id (заглушка) | `GlobalBeans().gpt` | пока то же |
| `IUserBehaviorStorePort` | История и популярность, соседи по каталогу | `StubBehaviorStore` | `PgRhythmData` |
| `IProductCatalogPort` | Карточка товара | `StubProductCatalog` | `PgRhythmData` |

`RecommendationService` получает ссылки на порты в конструкторе; тесты собирают те же сервисы на заглушках без поднятого сервера.

### POST `/action`

Хендлер проверяет заголовок и тело, отдаёт ошибки валидации. `ActionService` проверяет поля и допустимые `action_type` (`view`, `purchase`, `cart_add`, `like`, `save`), вызывает `TryReserve` на идемпотентности, при успехе сериализует JSON и вызывает `PublishJson` с routing key `user.action`. Повтор с тем же ключом в окне TTL не дублирует запись в очередь.

### GET `/recommend`

`RecommendationService`: по `IVisitLedger` решает, сбрасывать ли кэш ленты (долгий простой); при промахе кэша тянет историю и популярность из `IUserBehaviorStorePort`, GPT из `IGptRecommendPort`, блок «похожие» через `CatalogNeighborProductIds` с fallback на популярное; side-слои кэшируются отдельно. В реализации на Postgres история и глобальная популярность учитывают тип события (лайк и сохранение поднимают товар выше, чем один просмотр). Персональный merge источников в `.cpp` сервиса по-прежнему с фиксированными весами каналов; дифференциация по типу действия сосредоточена в SQL агрегатов.

### GET `/product`

`ProductService` запрашивает строку каталога у `IProductCatalogPort` и собирает JSON; при отсутствии id — пустой ответ на уровне хендлера.

### Воркер

Читает те же JSON, что ушли в Rabbit с `user.action`. Делает `INSERT … ON CONFLICT (user_id, idempotency_key) DO NOTHING` в `user_actions`. Если строка новая и собран с `RHYTHM_WORKER_WITH_REDIS`, дергается инвалидация связанных ключей в Redis (лента пользователя, производные кэши, отметка визита).

### Сборка и границы модулей

Флаги **`RHYTHM_WITH_*`** в `CMakeLists.txt` подключают исходники и линковку userver (`postgresql`, `redis`, `rabbitmq`). Бинарник `rhythm_service` не должен тянуть лишние `.cpp` реализаций. Хендлеры не зависят от этого напрямую — только от `port_resolution`.

---

## Сборка

Нужны Linux, CMake ≥ 3.16, собранный userver. Проще всего:

```bash
./scripts/bootstrap-userver.sh --install-deps   # Ubuntu 22.04/24.04, один раз
./scripts/bootstrap-userver.sh --build-only
```

Исходники userver — в `third_party/userver`, установка — в `.deps/userver`; оба пути в `.gitignore`. Если при `install(EXPORT …)` сыпались ошибки про `boost_*` после сборки без dev-пакетов, удалите `third_party/userver/build-release` и пересоберите после `--install-deps`.

Дополнительные фичи userver задаются так:

```bash
USERVER_BOOTSTRAP_CMAKE_ARGS='-DUSERVER_FEATURE_POSTGRESQL=ON -DUSERVER_FEATURE_REDIS=ON' \
  ./scripts/bootstrap-userver.sh --build-only
```

Дальше проект:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -Duserver_DIR="$PWD/.deps/userver/lib/cmake/userver"
cmake --build build -j"$(nproc)"
```

Флаги `rhythm` см. в `CMakeLists.txt`: `RHYTHM_WITH_POSTGRES`, `RHYTHM_WITH_REDIS`, `RHYTHM_WITH_RABBITMQ`, `RHYTHM_BUILD_WORKER`, `RHYTHM_WORKER_WITH_REDIS`. Userver должен быть собран с теми же `USERVER_FEATURE_*`, иначе линковка не пройдёт.

Запуск без внешних сервисов:

```bash
./build/rhythm_service --config configs/static_config.yaml
```

Postgres + Redis локально: `docker compose up -d`, конфиг `configs/static_config_pg_redis.yaml`. Миграции при первом создании тома БД выполняет `docker/init-db.sh` (из SQL убирается `CONCURRENTLY`, иначе init-скрипт официального образа падает).

Воркер (нужны rabbitmq + postgresql в userver):

```bash
cmake -S . -B build ... -DRHYTHM_BUILD_WORKER=ON
cmake --build build -j"$(nproc)"
./build/rhythm_worker --config configs/worker_static_config.yaml
```

## Тесты

По умолчанию собирается `rhythm_tests` (GTest). Если пакета нет, CMake может взять googletest через FetchContent (интернет при первом конфигурировании).

```bash
ctest --test-dir build --output-on-failure
```

Юнит-тесты гоняют сервисы, TTL, in-memory кэш, разбор JSON-полей и ключи Redis без поднятого HTTP и без живой БД. Отключить сборку тестов: `-DRHYTHM_BUILD_TESTS=OFF`.

## SQL

- `001_products_btree.sql` — таблица `products`
- `002_user_actions.sql` — `user_actions`, уникальность `(user_id, idempotency_key)`
- `003_user_actions_recommend_idx.sql` — индексы под запросы в `PgRhythmData` без `CONCURRENTLY` (удобно прогнать в одном сеансе)

В `001`/`002` индексы с `CONCURRENTLY` рассчитаны на ручной прогон вне транзакции init-файла.

## Конфиги

| Файл | Назначение |
|------|------------|
| `static_config.yaml` | только HTTP, всё в памяти |
| `static_config_pg_redis.yaml` | postgres + redis (рядом с compose) |
| `static_config_with_postgres.yaml` | только postgres |
| `static_config_with_redis.yaml` | только redis |
| `static_config_with_rabbit.yaml` | rabbit в HTTP |
| `static_config_stack.yaml` | полный стек |
| `worker_static_config*.yaml` | воркер |

Переменная окружения `SECDIST_CONFIG` может подменить JSON секретов.

## API (curl)

```bash
curl -sS -X POST "http://127.0.0.1:8080/action" \
  -H "Content-Type: application/json" \
  -H "Idempotency-Key: demo-key-1" \
  -d '{"user_id":"u1","action_type":"view","product_id":"42"}'

curl -sS "http://127.0.0.1:8080/recommend?user_id=u1"
curl -sS "http://127.0.0.1:8080/product?id=1"
```

## Прочее

Форматирование: `.clang-format` (Google, 120). Пример nginx для нескольких инстансов: `deploy/nginx-upstream.example.conf`. Очистка сборки: `rm -rf build`.
# Rhythm-service-base
