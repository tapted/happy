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

    void (*on_update)(void*, const Select&) = nullptr;
  };

  Select(Device& device, const char* object_id, const char* name, Config config,
         void* on_update_ctx = nullptr)
      : PersistentEntity(device, "select", object_id, name, true),
        config_(std::move(config)),
        on_update_ctx(on_update_ctx) {}

  bool empty() const { return config_.options.empty(); }
  std::string_view get_selected() const {
    return empty() ? "" : config_.options[state().selected_option_index_];
  }

  std::string get_discovery_payload() override;
  std::string get_state_payload() override;
  void handle_command(std::string_view payload) override;
  void on_change() override;

 private:
  Config config_;
  void* on_update_ctx = nullptr;
};

}  // namespace HAPPY::Entities