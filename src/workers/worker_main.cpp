#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/urabbitmq/component.hpp>

#ifdef RHYTHM_WORKER_WITH_REDIS
#include <userver/storages/redis/component.hpp>

#include "../infra/redis_rhythm_data.hpp"
#endif

#include "user_action_consumer.hpp"

int main(int argc, char* argv[]) {
    auto list = userver::components::MinimalComponentList()
                    .Append<userver::components::Secdist>()
                    .Append<userver::components::DefaultSecdistProvider>()
                    .Append<userver::clients::dns::Component>()
                    .Append<userver::components::Postgres>("rhythm-pg")
                    .Append<userver::components::RabbitMQ>("rhythm-rabbit");

#ifdef RHYTHM_WORKER_WITH_REDIS
    list.Append<userver::components::Redis>("rhythm-redis").Append<rhythm::infra::RedisRhythmData>();
#endif

    list.Append<rhythm::workers::UserActionConsumer>();

    return userver::utils::DaemonMain(argc, argv, list);
}
