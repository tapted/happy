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

void Light::handle_command(const unique_cjson& root_ptr) {
  // Wrap the raw pointer in our non-owning view
  JsonNodeView root(root_ptr.get());
  if (!root) return;

  bool state_changed = false;

  // 1. Check State (std::optional encapsulates the existence and type check)
  if (auto state = root["state"].as_string()) {
    is_on_ = (*state == "ON");
    state_changed = true;
  }

  // 2. Check Brightness
  if (auto bri = root["brightness"].as_int()) {
    brightness_ = *bri;
    state_changed = true;
  }

  // 3. Check RGB Color (Navigating into a nested object safely)
  if (auto color = root["color"]) {
    if (auto r = color["r"].as_int()) {
      r_ = *r;
      state_changed = true;
    }
    if (auto g = color["g"].as_int()) {
      g_ = *g;
      state_changed = true;
    }
    if (auto b = color["b"].as_int()) {
      b_ = *b;
      state_changed = true;
    }
  }

  // 4. Trigger the hardware update callback
  if (state_changed && config_.on_update) {
    config_.on_update(*this);

    // Acknowledge the new state back to Home Assistant
    std::string state_json = get_state_payload();
    device_.publish(*this);
  }
}

}  // namespace HAPPY::Entities