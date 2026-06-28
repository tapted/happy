#pragma once
#include <functional>
#include <span>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

class Select : public Entity {
 public:
  struct Config {
    const char* icon = "mdi:format-list-bulleted";
    const char* entity_category = "config";
    std::span<const char* const> options;
    std::function<void(const Select&)> on_update = nullptr;
  };

  Select(Device& device, std::string_view object_id, std::string_view name, Config config)
      : Entity(device, "select", object_id, name), config_(std::move(config)) {
    // Default to the first option if available
    if (!config_.options.empty()) {
      selected_option_ = config_.options.front();
    }
  }

  std::string_view get_selected() const { return selected_option_; }

  void initialize_topics() override { initialize_base_topics(true); }
  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;
  void handle_command(std::string_view payload) override;

 private:
  Config config_;
  std::string selected_option_;  // Safe to allocate once per selection change
};

}  // namespace HAPPY::Entities