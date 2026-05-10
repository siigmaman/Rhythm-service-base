cmake_minimum_required(VERSION 3.16)
project(rhythm_service)

option(RHYTHM_WITH_RABBITMQ "Link rhythm_service with RabbitMQ publisher (rhythm-event-publisher)" OFF)
option(RHYTHM_WITH_REDIS "Link rhythm_service with Redis (RedisRhythmData: cache + idempotency)" OFF)
option(RHYTHM_WITH_POSTGRES "Link rhythm_service with Postgres (PgRhythmData: catalog + user_actions)" OFF)
option(RHYTHM_BUILD_WORKER "Build rhythm_worker (needs userver rabbitmq + postgresql)" OFF)
option(RHYTHM_WORKER_WITH_REDIS "Worker: invalidate Redis after new user_actions row (needs redis + redis_rhythm_data in worker config)" OFF)

if(RHYTHM_WORKER_WITH_REDIS AND NOT RHYTHM_BUILD_WORKER)
  message(FATAL_ERROR "RHYTHM_WORKER_WITH_REDIS=ON requires RHYTHM_BUILD_WORKER=ON")
endif()

set(_RHYTHM_USERVER_COMPONENTS core)
if(RHYTHM_WITH_RABBITMQ OR RHYTHM_BUILD_WORKER)
  list(APPEND _RHYTHM_USERVER_COMPONENTS rabbitmq)
endif()
if(RHYTHM_BUILD_WORKER OR RHYTHM_WITH_POSTGRES)
  list(APPEND _RHYTHM_USERVER_COMPONENTS postgresql)
endif()
if(RHYTHM_WITH_REDIS OR (RHYTHM_BUILD_WORKER AND RHYTHM_WORKER_WITH_REDIS))
  list(APPEND _RHYTHM_USERVER_COMPONENTS redis)
endif()
find_package(userver REQUIRED COMPONENTS ${_RHYTHM_USERVER_COMPONENTS})

set(_RHYTHM_SERVICE_SOURCES
    src/domain/ttl_policy.cpp
    src/app/port_resolution.cpp
    src/app/component_bootstrap.cpp
    src/infra/in_memory_stubs.cpp
    src/infra/visit_ledger.cpp
    src/services/recommendation_service.cpp
    src/services/action_service.cpp
    src/services/product_service.cpp
    src/handlers/recommend_handler.cpp
    src/handlers/action_handler.cpp
    src/handlers/product_handler.cpp
    src/main.cpp
)
if(RHYTHM_WITH_RABBITMQ)
  list(APPEND _RHYTHM_SERVICE_SOURCES src/infra/rabbitmq_event_publisher.cpp)
endif()
if(RHYTHM_WITH_REDIS)
  list(APPEND _RHYTHM_SERVICE_SOURCES src/infra/redis_rhythm_data.cpp)
endif()
if(RHYTHM_WITH_POSTGRES)
  list(APPEND _RHYTHM_SERVICE_SOURCES src/infra/pg_rhythm_data.cpp)
endif()

add_executable(rhythm_service ${_RHYTHM_SERVICE_SOURCES})

target_include_directories(rhythm_service PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(rhythm_service PRIVATE userver::core)
target_compile_features(rhythm_service PRIVATE cxx_std_20)

if(RHYTHM_WITH_RABBITMQ)
  target_link_libraries(rhythm_service PRIVATE userver::rabbitmq)
  target_compile_definitions(rhythm_service PRIVATE RHYTHM_WITH_RABBITMQ=1)
endif()
if(RHYTHM_WITH_REDIS)
  target_link_libraries(rhythm_service PRIVATE userver::redis)
  target_compile_definitions(rhythm_service PRIVATE RHYTHM_WITH_REDIS=1)
endif()
if(RHYTHM_WITH_POSTGRES)
  target_link_libraries(rhythm_service PRIVATE userver::postgresql)
  target_compile_definitions(rhythm_service PRIVATE RHYTHM_WITH_POSTGRES=1)
endif()

if(RHYTHM_BUILD_WORKER)
  set(_RHYTHM_WORKER_SOURCES
      src/workers/worker_main.cpp
      src/workers/user_action_consumer.cpp
  )
  if(RHYTHM_WORKER_WITH_REDIS)
    list(APPEND _RHYTHM_WORKER_SOURCES src/infra/redis_rhythm_data.cpp)
  endif()
  add_executable(rhythm_worker ${_RHYTHM_WORKER_SOURCES})
  target_include_directories(rhythm_worker PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_link_libraries(rhythm_worker PRIVATE userver::rabbitmq userver::postgresql)
  target_compile_features(rhythm_worker PRIVATE cxx_std_20)
  if(RHYTHM_WORKER_WITH_REDIS)
    target_link_libraries(rhythm_worker PRIVATE userver::redis)
    target_compile_definitions(rhythm_worker PRIVATE RHYTHM_WORKER_WITH_REDIS=1)
  endif()
endif()

option(RHYTHM_BUILD_TESTS "Build unit tests (GTest)" ON)

if(RHYTHM_BUILD_TESTS)
  enable_testing()
  find_package(GTest CONFIG QUIET)
  if(NOT TARGET GTest::gtest_main)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()

  add_executable(
    rhythm_tests
    tests/ttl_policy_test.cpp
    tests/http_json_helpers_test.cpp
    tests/redis_keyspace_test.cpp
    tests/action_service_test.cpp
    tests/product_service_test.cpp
    tests/visit_ledger_test.cpp
    tests/in_memory_rhythm_data_test.cpp
    tests/recommendation_service_test.cpp
    src/domain/ttl_policy.cpp
    src/services/action_service.cpp
    src/services/product_service.cpp
    src/services/recommendation_service.cpp
    src/infra/in_memory_stubs.cpp
    src/infra/visit_ledger.cpp
  )
  target_include_directories(rhythm_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_link_libraries(rhythm_tests PRIVATE userver::core GTest::gtest_main)
  target_compile_features(rhythm_tests PRIVATE cxx_std_20)
  include(GoogleTest)
  gtest_discover_tests(rhythm_tests)
endif()
