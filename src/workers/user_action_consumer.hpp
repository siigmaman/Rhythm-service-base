#pragma once

#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/urabbitmq/consumer_component_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifdef RHYTHM_WORKER_WITH_REDIS
namespace rhythm::infra {
class RedisRhythmData;
}
#endif

namespace rhythm::workers {

/// Сообщения user.action -> user_actions; при вставке — инвалидация Redis, если собрано с RHYTHM_WORKER_WITH_REDIS.
class UserActionConsumer final : public userver::urabbitmq::ConsumerComponentBase {
   public:
    static constexpr std::string_view kName = "user-action-consumer";

    UserActionConsumer(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

    static userver::yaml_config::Schema GetStaticConfigSchema();

   protected:
    void Process(std::string message) override;

   private:
    userver::storages::postgres::ClusterPtr pg_;
#ifdef RHYTHM_WORKER_WITH_REDIS
    rhythm::infra::RedisRhythmData* redis_cache_{nullptr};
#endif
};

}  // namespace rhythm::workers
