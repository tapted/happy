#include "happy/entities/light.hpp"

#include <esp_log.h>

#include "espbase/json.hpp"
#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

bool Light::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;  // Max 32 entries.
  topic_buf_t command_topic;
  get_command_topic(command_topic);

  auto doc = stack_json(
      node("schema", "json"),                //
      node("command_topic", command_topic),  //
      node("optimistic", false),             //
      node_if("icon", config_.icon),         //
      node("supported_color_modes", stack_array(config_.supports_rgb ? "rgb" : "brightness")));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

std::string Light::get_state_payload() {
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