#pragma once

#include <string_view>

#include "happy/core/intrusive_list.hpp"

namespace HAPPY {

class Entity;  // Forward declaration

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
  template <typename Builder>
  void inject_into(Builder& builder) const {
    builder.with_object("device", [this](auto& dev) {
      dev.with_array("identifiers", [this](auto& arr) { arr.push(config_.identifiers); });
      dev.set("name", config_.name);

      if (config_.manufacturer) dev.set("manufacturer", config_.manufacturer);
      if (config_.model) dev.set("model", config_.model);
      if (config_.sw_version) dev.set("sw_version", config_.sw_version);
    });
  }

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