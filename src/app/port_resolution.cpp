#include "port_resolution.hpp"

#include "beans.hpp"
#ifdef RHYTHM_WITH_POSTGRES
#include "infra/pg_rhythm_data.hpp"
#endif
#ifdef RHYTHM_WITH_RABBITMQ
#include "infra/rabbitmq_event_publisher.hpp"
#endif
#ifdef RHYTHM_WITH_REDIS
#include "infra/redis_rhythm_data.hpp"
#endif

namespace rhythm::app {

#ifdef RHYTHM_WITH_REDIS
namespace {

rhythm::infra::RedisRhythmData& RedisBacking(const userver::components::ComponentContext& context) {
    return context.FindComponent<rhythm::infra::RedisRhythmData>();
}

}  // namespace
#endif

rhythm::infra::IEventQueuePort& EventQueue(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_RABBITMQ
    return context.FindComponent<rhythm::infra::RabbitMqEventPublisher>();
#else
    (void)context;
    return GlobalBeans().event_queue;
#endif
}

rhythm::infra::IIdempotencyPort& Idempotency(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_REDIS
    return RedisBacking(context);
#else
    (void)context;
    return GlobalBeans().rhythm_data;
#endif
}

rhythm::infra::IRecommendationCachePort& RecommendationCache(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_REDIS
    return RedisBacking(context);
#else
    (void)context;
    return GlobalBeans().rhythm_data;
#endif
}

rhythm::infra::ISideRecommendationCache& SideRecommendationCache(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_REDIS
    return RedisBacking(context);
#else
    (void)context;
    return GlobalBeans().rhythm_data;
#endif
}

rhythm::infra::IVisitLedger& VisitLedgerPort(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_REDIS
    return RedisBacking(context);
#else
    (void)context;
    return GlobalBeans().visit_ledger;
#endif
}

rhythm::infra::IGptRecommendPort& GptRecommend(const userver::components::ComponentContext& context) {
    (void)context;
    return GlobalBeans().gpt;
}

rhythm::infra::IUserBehaviorStorePort& BehaviorStore(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_POSTGRES
    return context.FindComponent<rhythm::infra::PgRhythmData>();
#else
    (void)context;
    return GlobalBeans().behavior_store;
#endif
}

rhythm::infra::IProductCatalogPort& ProductCatalog(const userver::components::ComponentContext& context) {
#ifdef RHYTHM_WITH_POSTGRES
    return context.FindComponent<rhythm::infra::PgRhythmData>();
#else
    (void)context;
    return GlobalBeans().product_catalog;
#endif
}

}  // namespace rhythm::app
