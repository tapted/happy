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
    void* context = nullptr;
    void (*on_update)(void* ctx, const Text&) = nullptr;
  };

  Text(Device& device, std::string_view object_id, std::string_view name, Config config)
      : PersistentEntity(device, "text", object_id, name), config_(config) {}

  std::string_view get_value() const { return state_.value; }

  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;
  void initialize_topics() override;
  void handle_command(std::string_view payload) override;

 private:
  Config config_;
};

} // namespace HAPPY::Entities