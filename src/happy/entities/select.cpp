
#include "happy/entities/select.hpp"

#include "espbase/json.h"
#include <esp_log.h>

namespace HAPPY::Entities {

void Select::initialize_topics() {
  initialize_base_topics(true);
}

std::string Select::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  builder.set("command_topic", command_topic_.c_str());
  if (config_.icon) builder.set("icon", config_.icon);
  if (config_.entity_category) builder.set("entity_category", config_.entity_category);

  builder.with_array("options", [this](auto& arr) {
    for (const char* opt : config_.options) {
      arr.push(opt);
    }
  });

  return doc.to_string();
}

std::string Select::get_state_payload() const {
  return std::string(get_selected());  // Select state is just the plain text string
}

void Select::handle_command(std::string_view payload) {
  // Validate the incoming payload against our allowed options
  for (size_t i = 0; i < config_.options.size(); ++i) {
    if (payload == config_.options[i]) {
      set_state({.selected_option_index_ = i});
      break;
    }
  }
}

void Select::on_change() {
  ESP_LOGI("Select", "State changed for %s: %s", object_id_.data(), get_state_payload().c_str());
  const size_t new_index = state().selected_option_index_;
  if (new_index != 0 && new_index >= config_.options.size()) {
    set_state({.selected_option_index_ = 0});  // Reset to a valid index if out of bounds
    return;  // Expect a recursive call.
  }

  if (config_.on_update) config_.on_update(*this);
}
}  // namespace HAPPY::Entities