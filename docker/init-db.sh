#!/usr/bin/env bash
# Postgres official image: init .sql выполняется в одной транзакции — CONCURRENTLY недопустим.
# Подаём файлы в psql по одному через stdin (автокоммит по операторам).
set -euo pipefail
for f in \
  /migrations/001_products_btree.sql \
  /migrations/002_user_actions.sql \
  /migrations/003_user_actions_recommend_idx.sql
do
  sed 's/CONCURRENTLY //g' "$f" | psql -v ON_ERROR_STOP=1 --username "$POSTGRES_USER" --dbname "$POSTGRES_DB"
done
