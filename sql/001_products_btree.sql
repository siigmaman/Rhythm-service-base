-- Карточка товара для GET /product?id= : B-tree по PK (BIGSERIAL) в Postgres по умолчанию — PRIMARY KEY.
-- Дополнительные индексы под выдачу и фильтры.

CREATE TABLE IF NOT EXISTS products (
    id          BIGSERIAL PRIMARY KEY,
    sku         VARCHAR(50) NOT NULL,
    title       VARCHAR(500) NOT NULL,
    category_id BIGINT,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    expires_at  TIMESTAMPTZ
);

CREATE UNIQUE INDEX CONCURRENTLY IF NOT EXISTS products_sku_uidx ON products (sku);

CREATE INDEX CONCURRENTLY IF NOT EXISTS products_category_created_idx
    ON products (category_id, created_at DESC)
    WHERE category_id IS NOT NULL;
