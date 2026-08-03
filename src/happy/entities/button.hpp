#pragma once

#include "happy/entity.hpp"

namespace HAPPY::Entities {

class Button : public Entity {
 public:
  struct Config {
    const char* icon = "mdi:gesture-tap-button";
    const char* device_class = nullptr;  // e.g., "restart", "update"
    const char* entity_category = "config";
    void (*on_press)(void*, const Button&) = nullptr;
  };

  Button(Device& device, const char* object_id, const char* name, Config config,
         void* ctx = nullptr)
      : Entity(device, "button", object_id, name, true), config_(std::move(config)), ctx_(ctx) {}

  std::string get_discovery_payload() override;
  void handle_command(std::string_view payload) override;

 private:
  Config config_;
  void* ctx_ = nullptr;
};

}  // namespace HAPPY::Entities