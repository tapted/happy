cmake_minimum_required(VERSION 3.24)
project(happy_tests CXX C)

# Enforce modern C++26
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG        v7.3.0
)

FetchContent_MakeAvailable(ArduinoJson)

enable_testing()

add_executable(happy_tests
    mocks/mock_mqtt.cpp
    entities/sensor_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/happy/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/happy/entity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/happy/transports/mqtt_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/happy/entities/sensor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/emulated/espbase/nvs_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/src/espbase/mac_address.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/src/espbase/stack_json/document.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/src/espbase/stack_json/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/src/espbase/stack_json/printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/src/espbase/stack_json/pretty_buffer.cpp
)

target_include_directories(happy_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../emulated/esp-idf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/emulated/esp-idf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../espbase/emulated
)

target_link_libraries(happy_tests PRIVATE
    gtest_main
    ArduinoJson
)

include(GoogleTest)
gtest_discover_tests(happy_tests)

