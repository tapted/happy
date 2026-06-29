#pragma once

#include <functional>
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

    // The lambda that fetches the real-time value
    std::function<std::string()> get_value = nullptr;
  };

  Sensor(Device& device, std::string_view object_id, std::string_view name, Config config)
      : Entity(device, "sensor", object_id, name), config_(std::move(config)) {}

  void initialize_topics() override;
  std::string get_discovery_payload() const override;

  // Evaluates the lambda to get the current ESP32 state
  std::string get_state_payload() const override {
    if (config_.get_value) return config_.get_value();
    return "unknown";
  }

 private:
  Config config_;
};

}  // namespace HAPPY::Entities