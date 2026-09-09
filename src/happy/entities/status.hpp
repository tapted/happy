#pragma once

#include "happy/entity.hpp"

namespace HAPPY::Entities {

class StaticStatus : public Entity {
 public:
  struct Config {
    const char* icon = "mdi:lan-connect";
    const char* entity_category = "diagnostic";
  };

  StaticStatus(Device& device, const char* object_id, const char* name, 
               const char* initial_state, Config config)
      : Entity(device, "sensor", object_id, name, false /* expects_commands */),
        config_(std::move(config)),
        current_state_(initial_state) {}

  // Update the state using a string literal pointer
  void set_state(const char* new_state);
  const char* get_state() const { return current_state_; }

  bool get_discovery_payload(sjson::Buffer& buffer) override;
  size_t get_state_payload(sjson::Buffer& buffer) override;

 private:
  Config config_;
  const char* current_state_;
};

}  // namespace HAPPY::Entities