#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "app/component_bootstrap.hpp"
#include "handlers/action_handler.hpp"
#include "handlers/product_handler.hpp"
#include "handlers/recommend_handler.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList();
    rhythm::app::AppendRhythmInfrastructure(component_list);

    component_list.Append<rhythm::handlers::ActionHandler>()
        .Append<rhythm::handlers::RecommendHandler>()
        .Append<rhythm::handlers::ProductHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
