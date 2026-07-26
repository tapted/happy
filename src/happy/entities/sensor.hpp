#pragma once

#include <string>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

class Sensor : public Entity {
 public:
  struct Config {
    const char* device_class = nullptr;
    const char* unit_of_measurement = nullptr;
    const char* icon = nullptr;
    const char* entity_category = "diagnostic";  // Default to diagnostic
    // TODO: Add a "state_class" field for sensors that have a state class.

    // The lambda that fetches the real-time value
    std::string (*get_value)(void*) = nullptr;
  };

  Sensor(Device& device, const char* object_id, const char* name, Config config,
         void* user_ctx = nullptr)
      : Entity(device, "sensor", object_id, name), config_(std::move(config)), user_ctx(user_ctx) {}

  std::string get_discovery_payload() override;

  // Evaluates the lambda to get the current ESP32 state
  std::string get_state_payload() override {
    if (config_.get_value) return config_.get_value(user_ctx);
    return "unknown";
  }

 private:
  // We copy config_. We could store a const reference (and delete the Sensor constructor that would
  // take a `Config&&` for safety). But it only saves 16 bytes per sensor and could be annoying to
  // force callers to make their configs static.
  const Config config_;
  void* user_ctx = nullptr;  // User context for the get_value lambda.
};

}  // namespace HAPPY::Entities