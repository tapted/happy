#include "happy/entities/light.hpp"

#include <esp_log.h>

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

bool Light::get_state_payload(sjson::Buffer& buffer) {
  auto color = path("color");
  auto doc = stack_json(node("state", state().is_on ? "ON" : "OFF"),  //
                        node("brightness", state().brightness),       //
                        node(color("r"), state().r),                  //
                        node(color("g"), state().g),                  //
                        node(color("b"), state().b)                   //
  );
  return doc.emit(buffer);
}

void Light::handle_command(const std::string_view payload) {
  auto state = this->state();
  auto color = path("color");
  std::string_view is_on;
  auto parser = json_parser(bind("state", is_on),                  //
                            bind("brightness", state.brightness),  //
                            bind(color("r"), state.r),             //
                            bind(color("g"), state.g),             //
                            bind(color("b"), state.b));
  parser.parse(payload);
  state.is_on = (is_on == "ON");

  ESP_LOGD("Light", "Command received for %s: %.*s", object_id_, static_cast<int>(payload.length()),
           payload.data());
  ESP_LOGD("Light", "New state: is_on=%d, brightness=%d, r=%d, g=%d, b=%d", state.is_on,
           state.brightness, state.r, state.g, state.b);
  set_state(state);
}

}  // namespace HAPPY::Entities