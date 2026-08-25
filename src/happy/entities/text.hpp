#pragma once

#include <string_view>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct TextState {
  char value[64]{};
};

class Text : public PersistentEntity<Text, TextState> {
 public:
  struct Config {
    const char* icon = "mdi:form-textbox";
    const char* entity_category = "config";
    void (*on_update)(const Text&) = nullptr;
  };

  Text(Device& device, const char* object_id, const char* name, Config config,
       void* on_update_ctx = nullptr)
      : PersistentEntity(device, "text", object_id, name, true /* expects_commands */),
        config_(config),
        on_update_ctx(on_update_ctx) {}

  void set_value(std::string_view new_value) {
    TextState new_state = state();
    snprintf(new_state.value, sizeof(new_state.value), "%.*s", (int)new_value.size(),
             new_value.data());
    set_state(new_state);
  }
  std::string_view get_value() const { return state().value; }

  bool get_discovery_payload(sjson::Buffer& buffer) override;
  size_t get_state_payload(sjson::Buffer& buffer) override;
  void handle_command(std::string_view payload) override;

  void on_change() override {
    if (config_.on_update) config_.on_update(*this);
  }

 private:
  Config config_;
  void* on_update_ctx = nullptr;
};

}  // namespace HAPPY::Entities