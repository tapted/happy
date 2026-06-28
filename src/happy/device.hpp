#pragma once

#include <string_view>

#include "espbase/json_fwd.h"
#include "happy/core/intrusive_list.hpp"

namespace HAPPY {

class Entity;  // Forward declaration

// A Device represents a physical or virtual device that can have multiple entities (sensors,
// lights, buttons, etc.) associated with it.
// Sample usage:
// ```cpp
// HAPPY::Transports::MqttDevice dongley_device({
//     .identifiers = "dongley_v1_001",
//     .name = "Dongley",
//     .manufacturer = "Custom",
//     .model = "ESP32-S3 WROOM-1 DevKit",
//     .sw_version = "0.1",  // esp_app_get_description()->version
// });
// ```
class Device {
 public:
  struct Config {
    const char* identifiers;
    const char* name;
    const char* manufacturer = "Custom";
    const char* model = "ESP32 Device";
    const char* sw_version = nullptr;
  };

  explicit constexpr Device(const Config& config) : config_(config) {}

  const char* get_identifier() const { return config_.identifiers; }
  const char* get_name() const { return config_.name; }

  // Injects the HA "device" grouping block into an existing json.h builder
  void inject_into(JsonObjectBuilder& builder) const;

  // --- Registry Implementation ---
  void register_entity(Entity* entity);
  virtual void publish(const Entity& entity) const = 0;
  void begin();

  // Parses JSON and routes it to the matching entity
  void dispatch_command(std::string_view topic, std::string_view payload) const;

 protected:
  Config config_;
  Core::IntrusiveList<Entity> entities_;
};

}  // namespace HAPPY