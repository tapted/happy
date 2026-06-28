#include "happy/entities/light.hpp"

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

  builder.set("state", is_on_ ? "ON" : "OFF");
  builder.set("brightness", brightness_);
  builder.with_object("color", [&](auto& color) {
    color.set("r", r_);
    color.set("g", g_);
    color.set("b", b_);
  });

  return doc.to_string();
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

  if (!state_changed) return;

  device_.publish(*this);
  save_state();
  if (config_.on_update) config_.on_update(*this);
}
}  // namespace HAPPY::Entities