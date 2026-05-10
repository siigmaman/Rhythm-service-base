#include "user_action_consumer.hpp"

#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/query.hpp>

#ifdef RHYTHM_WORKER_WITH_REDIS
#include "../infra/redis_rhythm_data.hpp"
#else
#include "../infra/redis_keyspace.hpp"
#endif

namespace rhythm::workers {

namespace {

const userver::storages::Query kInsertUserAction{
    "INSERT INTO user_actions (user_id, action_type, product_id, idempotency_key) "
    "VALUES ($1, $2, $3, $4) ON CONFLICT (user_id, idempotency_key) DO NOTHING",
};

#ifndef RHYTHM_WORKER_WITH_REDIS
void LogCacheInvalidation(const std::string& user_id) {
    using rhythm::infra::redis_keys::FeedKey;
    using rhythm::infra::redis_keys::GptRankKey;
    using rhythm::infra::redis_keys::VisitLastKey;
    LOG_INFO() << "cache.invalidate would DEL feed=" << FeedKey(user_id) << " gpt_rank=" << GptRankKey(user_id)
               << " visit_last=" << VisitLastKey(user_id);
}
#endif

}  // namespace

UserActionConsumer::UserActionConsumer(const userver::components::ComponentConfig& config,
                                       const userver::components::ComponentContext& context)
    : userver::urabbitmq::ConsumerComponentBase(config, context),
      pg_(context.FindComponent<userver::components::Postgres>(config["postgres_name"].As<std::string>()).GetCluster())
#ifdef RHYTHM_WORKER_WITH_REDIS
      ,
      redis_cache_(&context.FindComponent<rhythm::infra::RedisRhythmData>(
          config["redis_cache_component"].As<std::string>("rhythm-redis-data")))
#endif
{
}

userver::yaml_config::Schema UserActionConsumer::GetStaticConfigSchema() {
#ifdef RHYTHM_WORKER_WITH_REDIS
    return userver::yaml_config::MergeSchemas<userver::urabbitmq::ConsumerComponentBase>(R"(
type: object
description: RabbitMQ consumer for rhythm user actions
additionalProperties: false
properties:
    postgres_name:
        type: string
        description: Name of components::Postgres in static config
    redis_cache_component:
        type: string
        description: Name of rhythm::infra::RedisRhythmData (InvalidateAfterUserAction)
        defaultDescription: rhythm-redis-data
)");
#else
    return userver::yaml_config::MergeSchemas<userver::urabbitmq::ConsumerComponentBase>(R"(
type: object
description: RabbitMQ consumer for rhythm user actions
additionalProperties: false
properties:
    postgres_name:
        type: string
        description: Name of components::Postgres in static config
)");
#endif
}

void UserActionConsumer::Process(std::string message) {
    userver::formats::json::Value json;
    try {
        json = userver::formats::json::FromString(message);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "user_action consumer: invalid json, ack and skip: " << ex.what();
        return;
    }

    if (!json.HasMember("user_id") || !json.HasMember("action_type") || !json.HasMember("product_id") ||
        !json.HasMember("idempotency_key")) {
        LOG_ERROR() << "user_action consumer: missing required fields, ack and skip";
        return;
    }

    const auto user_id = json["user_id"].As<std::string>();
    const auto action_type = json["action_type"].As<std::string>();
    const auto product_id = json["product_id"].As<std::string>();
    const auto idempotency_key = json["idempotency_key"].As<std::string>();

    const auto res = pg_->Execute(userver::storages::postgres::ClusterHostType::kMaster, kInsertUserAction, user_id,
                                  action_type, product_id, idempotency_key);

    if (res.RowsAffected() > 0) {
#ifdef RHYTHM_WORKER_WITH_REDIS
        redis_cache_->InvalidateAfterUserAction(user_id);
        LOG_INFO() << "user_action consumer: redis invalidated after insert user_id=" << user_id;
#else
        LogCacheInvalidation(user_id);
#endif
    } else {
        LOG_INFO() << "user_action consumer: duplicate (user_id,idempotency_key), no cache invalidation";
    }
}

}  // namespace rhythm::workers
