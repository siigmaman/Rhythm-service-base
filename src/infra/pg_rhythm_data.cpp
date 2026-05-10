#include "pg_rhythm_data.hpp"

#include <limits>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/query.hpp>

namespace rhythm::infra {

namespace {

using userver::storages::postgres::ClusterHostType;
using userver::storages::postgres::Query;

const Query kSelectProduct{
    "SELECT id, sku, title FROM products WHERE id = $1",
    Query::Name{"rhythm_select_product_by_id"},
};

// Окна и типы согласованы с валидацией POST /action. Веса — чтобы лайк/сохранение сильнее
// влияли на порядок истории и на глобальную популярность, чем простой просмотр.
constexpr int kRecentHistoryDays = 90;
constexpr int kPopularWindowDays = 30;

const Query kRecentProductIds{
    "SELECT pid FROM ("
    "  SELECT DISTINCT ON (CAST(product_id AS BIGINT))"
    "    CAST(product_id AS BIGINT) AS pid,"
    "    created_at,"
    "    CASE action_type"
    "      WHEN 'save' THEN 4"
    "      WHEN 'purchase' THEN 4"
    "      WHEN 'like' THEN 3"
    "      WHEN 'cart_add' THEN 2"
    "      WHEN 'view' THEN 1"
    "      ELSE 0"
    "    END AS type_rank"
    "  FROM user_actions"
    "  WHERE user_id = $1"
    "    AND product_id ~ '^[0-9]+$'"
    "    AND action_type IN ('view', 'purchase', 'cart_add', 'like', 'save')"
    "    AND created_at >= now() - ($3::bigint * interval '1 day')"
    "  ORDER BY CAST(product_id AS BIGINT), created_at DESC"
    ") sub ORDER BY type_rank DESC, created_at DESC LIMIT $2",
    Query::Name{"rhythm_recent_product_ids"},
};

const Query kPopularProductIds{
    "SELECT CAST(product_id AS BIGINT) AS pid"
    " FROM user_actions"
    " WHERE product_id ~ '^[0-9]+$'"
    "   AND action_type IN ('view', 'purchase', 'cart_add', 'like', 'save')"
    "   AND created_at >= now() - ($2::bigint * interval '1 day')"
    " GROUP BY CAST(product_id AS BIGINT)"
    " ORDER BY SUM("
    "   CASE action_type"
    "     WHEN 'save' THEN 3"
    "     WHEN 'purchase' THEN 3"
    "     WHEN 'like' THEN 2"
    "     WHEN 'cart_add' THEN 2"
    "     WHEN 'view' THEN 1"
    "     ELSE 0"
    "   END"
    " ) DESC, pid ASC"
    " LIMIT $1",
    Query::Name{"rhythm_popular_product_ids"},
};

const Query kCatalogNeighborIds{
    "SELECT p.id AS pid FROM products p"
    " WHERE p.category_id IS NOT NULL"
    "   AND p.category_id = (SELECT p2.category_id FROM products p2 WHERE p2.id = $1)"
    "   AND p.id <> $1"
    " ORDER BY p.id ASC"
    " LIMIT $2",
    Query::Name{"rhythm_catalog_neighbor_ids"},
};

bool IsReasonableLimit(std::size_t limit) {
    return limit > 0 && limit <= static_cast<std::size_t>(std::numeric_limits<int>::max());
}

std::vector<std::int64_t> PidsFromPidColumn(const userver::storages::postgres::ResultSet& res) {
    std::vector<std::int64_t> out;
    out.reserve(res.Size());
    for (const auto& row : res) {
        out.push_back(row["pid"].As<std::int64_t>());
    }
    return out;
}

}  // namespace

PgRhythmData::PgRhythmData(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>(config["postgres_name"].As<std::string>("rhythm-pg"))
              .GetCluster()) {}

std::optional<ProductRow> PgRhythmData::GetById(std::int64_t id) const {
    const auto res = pg_->Execute(ClusterHostType::kSlave, kSelectProduct, id);
    if (res.IsEmpty()) return std::nullopt;
    const auto& row = res[0];
    return ProductRow{
        row["id"].As<std::int64_t>(),
        row["sku"].As<std::string>(),
        row["title"].As<std::string>(),
    };
}

std::vector<std::int64_t> PgRhythmData::RecentProductIds(const std::string& user_id, std::size_t limit) const {
    if (!IsReasonableLimit(limit)) return {};
    const auto res = pg_->Execute(ClusterHostType::kSlave, kRecentProductIds, user_id, static_cast<int>(limit),
                                  static_cast<std::int64_t>(kRecentHistoryDays));
    return PidsFromPidColumn(res);
}

std::vector<std::int64_t> PgRhythmData::PopularProductIds(std::size_t limit) const {
    if (!IsReasonableLimit(limit)) return {};
    const auto res = pg_->Execute(ClusterHostType::kSlave, kPopularProductIds, static_cast<int>(limit),
                                  static_cast<std::int64_t>(kPopularWindowDays));
    return PidsFromPidColumn(res);
}

std::vector<std::int64_t> PgRhythmData::CatalogNeighborProductIds(std::int64_t anchor_product_id,
                                                                  std::size_t limit) const {
    if (!IsReasonableLimit(limit) || anchor_product_id <= 0) return {};
    const auto res =
        pg_->Execute(ClusterHostType::kSlave, kCatalogNeighborIds, anchor_product_id, static_cast<int>(limit));
    return PidsFromPidColumn(res);
}

userver::yaml_config::Schema PgRhythmData::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Postgres-backed product catalog and user_actions aggregates
additionalProperties: false
properties:
    postgres_name:
        type: string
        description: Name of components::Postgres in static config
        defaultDescription: rhythm-pg
)");
}

}  // namespace rhythm::infra
