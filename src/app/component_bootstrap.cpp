#include "component_bootstrap.hpp"

#if defined(RHYTHM_WITH_RABBITMQ) || defined(RHYTHM_WITH_REDIS) || defined(RHYTHM_WITH_POSTGRES)
#include <userver/clients/dns/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#endif

#ifdef RHYTHM_WITH_POSTGRES
#include <userver/storages/postgres/component.hpp>

#include "infra/pg_rhythm_data.hpp"
#endif

#ifdef RHYTHM_WITH_REDIS
#include <userver/storages/redis/component.hpp>

#include "infra/redis_rhythm_data.hpp"
#endif

#ifdef RHYTHM_WITH_RABBITMQ
#include <userver/urabbitmq/component.hpp>

#include "infra/rabbitmq_event_publisher.hpp"
#endif

namespace rhythm::app {

void AppendRhythmInfrastructure([[maybe_unused]] userver::components::ComponentList& component_list) {
#if defined(RHYTHM_WITH_RABBITMQ) || defined(RHYTHM_WITH_REDIS) || defined(RHYTHM_WITH_POSTGRES)
    component_list.Append<userver::clients::dns::Component>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>();
#endif

#ifdef RHYTHM_WITH_POSTGRES
    component_list.Append<userver::components::Postgres>("rhythm-pg").Append<rhythm::infra::PgRhythmData>();
#endif

#ifdef RHYTHM_WITH_REDIS
    component_list.Append<userver::components::Redis>("rhythm-redis").Append<rhythm::infra::RedisRhythmData>();
#endif

#ifdef RHYTHM_WITH_RABBITMQ
    component_list.Append<userver::components::RabbitMQ>("rhythm-rabbit")
        .Append<rhythm::infra::RabbitMqEventPublisher>();
#endif
}

}  // namespace rhythm::app
