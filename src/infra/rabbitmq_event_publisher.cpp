#include "rabbitmq_event_publisher.hpp"

#include <chrono>
#include <stdexcept>

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/logging/log.hpp>
#include <userver/urabbitmq/component.hpp>

namespace rhythm::infra {

namespace {

userver::urabbitmq::Exchange::Type ParseExchangeType(const std::string& s) {
    if (s == "fanout") return userver::urabbitmq::Exchange::Type::kFanOut;
    if (s == "direct") return userver::urabbitmq::Exchange::Type::kDirect;
    if (s == "topic") return userver::urabbitmq::Exchange::Type::kTopic;
    throw std::runtime_error("rhythm-event-publisher: unknown exchange_type '" + s + "'");
}

}  // namespace

RabbitMqEventPublisher::RabbitMqEventPublisher(const userver::components::ComponentConfig& config,
                                               const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      client_(
          context.FindComponent<userver::components::RabbitMQ>(config["rabbit_name"].As<std::string>()).GetClient()),
      exchange_(config["exchange"].As<std::string>()),
      exchange_type_(ParseExchangeType(config["exchange_type"].As<std::string>("topic"))),
      queue_(config["queue"].As<std::string>()),
      bind_routing_key_(config["bind_routing_key"].As<std::string>("user.action")) {}

void RabbitMqEventPublisher::OnAllComponentsLoaded() {
    userver::components::ComponentBase::OnAllComponentsLoaded();
    const auto deadline = userver::engine::Deadline::FromDuration(std::chrono::seconds{10});
    auto admin = client_->GetAdminChannel(deadline);
    admin.DeclareExchange(exchange_, exchange_type_, deadline);
    admin.DeclareQueue(queue_, deadline);
    admin.BindQueue(exchange_, queue_, bind_routing_key_, deadline);
    LOG_INFO() << "rhythm-event-publisher: topology ready exchange=" << exchange_.GetUnderlying()
               << " queue=" << queue_.GetUnderlying() << " bind_rk=" << bind_routing_key_;
}

void RabbitMqEventPublisher::PublishJson(const std::string& routing_key, const std::string& json_payload) {
    const auto deadline = userver::engine::Deadline::FromDuration(std::chrono::seconds{5});
    client_->PublishReliable(exchange_, routing_key, json_payload, userver::urabbitmq::MessageType::kTransient,
                             deadline);
    LOG_INFO() << "rabbit.publish rk=" << routing_key << " bytes=" << json_payload.size();
}

userver::yaml_config::Schema RabbitMqEventPublisher::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Publish user events to RabbitMQ (matches rhythm_worker queue binding)
additionalProperties: false
properties:
    rabbit_name:
        type: string
        description: Name of components::RabbitMQ in static config
    exchange:
        type: string
        description: AMQP exchange name
    exchange_type:
        type: string
        description: fanout | direct | topic
        defaultDescription: topic
    queue:
        type: string
        description: Queue to declare and bind (same as worker consumes)
    bind_routing_key:
        type: string
        description: Routing key for queue binding (HTTP publisher still sends message rk from IEventQueuePort)
        defaultDescription: user.action
)");
}

}  // namespace rhythm::infra
