#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/urabbitmq/client.hpp>
#include <userver/urabbitmq/typedefs.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include "ports.hpp"

namespace rhythm::infra {

/// JSON в RabbitMQ; при старте объявляет exchange, очередь и привязку к воркеру.
class RabbitMqEventPublisher final : public userver::components::ComponentBase, public IEventQueuePort {
   public:
    static constexpr std::string_view kName = "rhythm-event-publisher";

    RabbitMqEventPublisher(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context);

    void PublishJson(const std::string& routing_key, const std::string& json_payload) override;

    void OnAllComponentsLoaded() override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

   private:
    std::shared_ptr<userver::urabbitmq::Client> client_;
    userver::urabbitmq::Exchange exchange_;
    userver::urabbitmq::Exchange::Type exchange_type_;
    userver::urabbitmq::Queue queue_;
    std::string bind_routing_key_;
};

}  // namespace rhythm::infra
