#pragma once

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct SwitchState {
  bool is_on = false;
};

class Switch : public PersistentEntity<Switch, SwitchState> {
 public:
  struct Config {
    const char* icon = nullptr;          // HA provides a default switch icon
    const char* device_class = nullptr;  // e.g., "outlet", "switch"
    const char* entity_category = nullptr;
    void (*on_change)(void*, const Switch&) = nullptr;
  };

  Switch(Device& device, const char* object_id, const char* name, Config config,
         void* ctx = nullptr)
      : PersistentEntity(device, "switch", object_id, name, true /* expects_commands */),
        config_(std::move(config)),
        ctx_(ctx) {}

  size_t get_state_payload(sjson::Buffer& buffer) override;
  bool get_discovery_payload(sjson::Buffer& buffer) override;
  void handle_command(std::string_view payload) override;

  // Programmatic controls for internal firmware logic
  void turn_on();
  void turn_off();
  void toggle();
  bool is_on() const { return state().is_on; }

 protected:
  // Triggered automatically by PersistentEntity when set_state() changes the value,
  // or when the initial state is loaded from NVS during boot.
  void on_change() override;

 private:
  Config config_;
  void* ctx_ = nullptr;
};

}  // namespace HAPPY::Entities