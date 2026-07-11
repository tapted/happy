#pragma once
#include <functional>
#include <span>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct SelectState {
  size_t selected_option_index_ = 0;
};

class Select : public PersistentEntity<Select, SelectState> {
 public:
  struct Config {
    const char* icon = "mdi:format-list-bulleted";
    const char* entity_category = "config";
    std::span<const char* const> options;
    std::function<void(const Select&)> on_update = nullptr;
  };

  Select(Device& device, std::string_view object_id, std::string_view name, Config config)
      : PersistentEntity(device, "select", object_id, name), config_(std::move(config)) {}

  bool empty() const { return config_.options.empty(); }
  std::string_view get_selected() const {
    return empty() ? "" : config_.options[state().selected_option_index_];
  }

  void initialize_topics() override;
  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;
  void handle_command(std::string_view payload) override;
  void on_change() override;

 private:
  Config config_;
};

}  // namespace HAPPY::Entities