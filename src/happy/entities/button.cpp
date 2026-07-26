#include "happy/entities/button.hpp"

#include "espbase/json.hpp"

namespace HAPPY::Entities {
  
std::string Button::get_discovery_payload() {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  topic_buf_t command_topic;
  this->get_command_topic(command_topic);
  builder.set("command_topic", (const char*)command_topic);

  if (config_.icon) builder.set("icon", config_.icon);
  if (config_.device_class) builder.set("device_class", config_.device_class);
  if (config_.entity_category) builder.set("entity_category", config_.entity_category);

  return doc.to_string();
}

void Button::handle_command(std::string_view payload) {
  if (payload == "PRESS" && config_.on_press) {
    config_.on_press(*this);
  }
}

}  // namespace HAPPY::Entities