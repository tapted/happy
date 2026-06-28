
#include "happy/entities/select.hpp"

#include "espbase/json.h"

namespace HAPPY::Entities {

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
  return selected_option_;  // Select state is just the plain text string
}

void Select::handle_command(std::string_view payload) {
  // Validate the incoming payload against our allowed options
  for (const char* opt : config_.options) {
    if (payload == opt) {
      selected_option_ = opt;
      if (config_.on_update) config_.on_update(*this);
      device_.publish(*this);
      return;
    }
  }
}

}  // namespace HAPPY::Entities