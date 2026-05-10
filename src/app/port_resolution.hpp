#pragma once

#include <userver/components/component_context.hpp>

#include "infra/ports.hpp"
#include "infra/visit_ledger.hpp"

namespace rhythm::app {

[[nodiscard]] rhythm::infra::IEventQueuePort& EventQueue(const userver::components::ComponentContext& context);
[[nodiscard]] rhythm::infra::IIdempotencyPort& Idempotency(const userver::components::ComponentContext& context);

[[nodiscard]] rhythm::infra::IRecommendationCachePort& RecommendationCache(
    const userver::components::ComponentContext& context);
[[nodiscard]] rhythm::infra::ISideRecommendationCache& SideRecommendationCache(
    const userver::components::ComponentContext& context);
[[nodiscard]] rhythm::infra::IVisitLedger& VisitLedgerPort(const userver::components::ComponentContext& context);

[[nodiscard]] rhythm::infra::IGptRecommendPort& GptRecommend(const userver::components::ComponentContext& context);
[[nodiscard]] rhythm::infra::IUserBehaviorStorePort& BehaviorStore(
    const userver::components::ComponentContext& context);
[[nodiscard]] rhythm::infra::IProductCatalogPort& ProductCatalog(const userver::components::ComponentContext& context);

}  // namespace rhythm::app
