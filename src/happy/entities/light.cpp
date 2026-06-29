#include "happy/entities/light.hpp"

#include <esp_log.h>

#include "espbase/json.h"

namespace HAPPY::Entities {

std::string Light::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());

  this->inject_base_config(builder);

  builder.set("schema", "json");
  builder.set("command_topic", command_topic_.c_str());
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

  builder.set("state", state_.is_on ? "ON" : "OFF");
  builder.set("brightness", state_.brightness);
  builder.with_object("color", [&](auto& color) {
    color.set("r", state_.r);
    color.set("g", state_.g);
    color.set("b", state_.b);
  });

  return doc.to_string();
}

void Light::initialize_topics() {
  bool loaded_from_nvs = initialize_base_topics(true);
  ESP_LOGD("Light",
           "Initialized topics for %s (loaded_from_nvs=%d): discovery=%s, state=%s, command=%s, "
           "state_payload=%s",
           object_id_.data(), loaded_from_nvs, discovery_topic_.c_str(), state_topic_.c_str(),
           command_topic_.c_str(), get_state_payload().c_str());
  if (loaded_from_nvs && config_.on_update) config_.on_update(*this);
}

void Light::handle_command(const std::string_view payload) {
  unique_cjson root_ptr{cJSON_ParseWithLength(payload.data(), payload.length())};

  // Wrap the raw pointer in our non-owning view
  JsonNodeView root(root_ptr.get());
  if (!root) return;

  bool state_changed = false;

  state_changed |= root.change(state_.is_on, "state", [](auto s) { return s == "ON"; });
  state_changed |= root.change(state_.brightness, "brightness");
  if (auto color = root["color"]) {
    state_changed |= color.change(state_.r, "r");
    state_changed |= color.change(state_.g, "g");
    state_changed |= color.change(state_.b, "b");
  }

  ESP_LOGD("Light", "Command received for %s: %.*s", object_id_.data(),
           static_cast<int>(payload.length()), payload.data());
  ESP_LOGD("Light", "New state: is_on=%d, brightness=%d, r=%d, g=%d, b=%d (changed=%d)",
           state_.is_on, state_.brightness, state_.r, state_.g, state_.b, state_changed);

  if (!state_changed) return;

  device_.publish(*this);
  save_state();
  if (config_.on_update) config_.on_update(*this);
}
}  // namespace HAPPY::Entities