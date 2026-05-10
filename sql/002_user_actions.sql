-- События из очереди `user.action` (воркер пишет сюда). Индексы под историю для /recommend и аналитику.

CREATE TABLE IF NOT EXISTS user_actions (
    id                BIGSERIAL PRIMARY KEY,
    user_id           TEXT NOT NULL,
    action_type       TEXT NOT NULL,
    product_id        TEXT NOT NULL,
    idempotency_key   TEXT NOT NULL,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX CONCURRENTLY IF NOT EXISTS user_actions_user_id_idem_uidx
    ON user_actions (user_id, idempotency_key);

CREATE INDEX CONCURRENTLY IF NOT EXISTS user_actions_user_created_idx
    ON user_actions (user_id, created_at DESC);

CREATE INDEX CONCURRENTLY IF NOT EXISTS user_actions_product_created_idx
    ON user_actions (product_id, created_at DESC);
