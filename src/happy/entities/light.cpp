#include "happy/entities/light.hpp"

#include <esp_log.h>

#include "espbase/json.h"

namespace HAPPY::Entities {

std::string Light::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());

  this->inject_base_config(builder);

  builder.set("schema", "json");

  topic_buf_t command_topic;
  get_command_topic(command_topic);
  builder.set("command_topic", (const char*)command_topic);

  builder.set("optimistic", false);

  if (config_.icon) builder.set("icon", config_.icon);

  builder.with_array("supported_color_modes", [this](auto& arr) {
    if (config_.supports_rgb)
      arr.push("rgb");
    else
      arr.push("brightness");
  });

  return doc.to_string();
}

std::string Light::get_state_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());

  builder.set("state", state().is_on ? "ON" : "OFF");
  builder.set("brightness", state().brightness);
  builder.with_object("color", [&](auto& color) {
    color.set("r", state().r);
    color.set("g", state().g);
    color.set("b", state().b);
  });

  return doc.to_string();
}

void Light::handle_command(const std::string_view payload) {
  unique_cjson root_ptr{cJSON_ParseWithLength(payload.data(), payload.length())};

  // Wrap the raw pointer in our non-owning view
  JsonNodeView root(root_ptr.get());
  if (!root) return;

  auto state = this->state();

  root.change(state.is_on, "state", [](auto s) { return s == "ON"; });
  root.change(state.brightness, "brightness");
  if (auto color = root["color"]) {
    color.change(state.r, "r");
    color.change(state.g, "g");
    color.change(state.b, "b");
  }

  ESP_LOGD("Light", "Command received for %s: %.*s", object_id_, static_cast<int>(payload.length()),
           payload.data());
  ESP_LOGD("Light", "New state: is_on=%d, brightness=%d, r=%d, g=%d, b=%d", state.is_on,
           state.brightness, state.r, state.g, state.b);
  set_state(state);
}
}  // namespace HAPPY::Entities