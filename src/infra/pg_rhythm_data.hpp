#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "ports.hpp"

namespace rhythm::infra {

/// Чтение `products` и агрегатов по `user_actions` для HTTP (`/product`, `/recommend`).
class PgRhythmData final : public userver::components::ComponentBase,
                           public IProductCatalogPort,
                           public IUserBehaviorStorePort {
   public:
    static constexpr std::string_view kName = "rhythm-pg-data";

    PgRhythmData(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);

    std::optional<ProductRow> GetById(std::int64_t id) const override;

    std::vector<std::int64_t> RecentProductIds(const std::string& user_id, std::size_t limit) const override;
    std::vector<std::int64_t> PopularProductIds(std::size_t limit) const override;
    std::vector<std::int64_t> CatalogNeighborProductIds(std::int64_t anchor_product_id,
                                                        std::size_t limit) const override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

   private:
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace rhythm::infra
