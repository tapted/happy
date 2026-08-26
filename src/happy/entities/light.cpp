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
      node("brightness", true),              //  Light supports brightness with rgb
      node("color_mode", true),              //
      node("supported_color_modes", stack_array(config_.supports_rgb ? "rgb" : "brightness")),
      node_if(!config_.effect_list.empty(), "effect", true),
      node_if(!config_.effect_list.empty(), "effect_list", sjson::span_array(config_.effect_list)));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

size_t Light::get_state_payload(sjson::Buffer& buffer) {
  auto color = stack_json(node("r", state().r), node("g", state().g), node("b", state().b));
  auto doc = stack_json(node("state", state().is_on ? "ON" : "OFF"),                      //
                        node("color_mode", config_.supports_rgb ? "rgb" : "brightness"),  //
                        node("brightness", state().brightness),                           //
                        node_if(config_.supports_rgb, "color", color)                     //
  );
  return doc.emit(buffer);
}

void Light::handle_command(const std::string_view payload) {
  auto state = this->state();
  auto color = path("color");
  std::string_view is_on;
  std::string_view effect;
  std::string_view flash;
  auto parser = json_parser(bind("state", is_on),                  //
                            bind("brightness", state.brightness),  //
                            bind("effect", effect),
                            bind("flash", flash),
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

  if (!effect.empty() && config_.on_effect) config_.on_effect(*this, effect);
  if (!flash.empty() && config_.on_flash) config_.on_flash(*this, flash);
}

}  // namespace HAPPY::Entities