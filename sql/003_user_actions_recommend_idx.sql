-- Индексы под запросы PgRhythmData (история / популярность по action_type и времени).
-- Без CONCURRENTLY — можно выполнять в одной транзакции с другим DDL в CI.

CREATE INDEX IF NOT EXISTS user_actions_user_action_time_idx
    ON user_actions (user_id, action_type, created_at DESC);

CREATE INDEX IF NOT EXISTS user_actions_action_time_product_idx
    ON user_actions (action_type, created_at DESC, product_id);

CREATE INDEX IF NOT EXISTS products_category_id_idx
    ON products (category_id, id);
