#pragma once

// Регистрация Postgres / Redis / Rabbit в ComponentList при флагах CMake.

#include <userver/components/component_list.hpp>

namespace rhythm::app {

void AppendRhythmInfrastructure([[maybe_unused]] userver::components::ComponentList& component_list);

}  // namespace rhythm::app
