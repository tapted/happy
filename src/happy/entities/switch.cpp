#include "happy/entities/switch.hpp"

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

size_t Switch::get_state_payload(sjson::Buffer& buffer) {
  // Home Assistant expects standard "ON" and "OFF" strings for switch states
  return buffer.write(state().is_on ? "ON" : "OFF");
}

bool Switch::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;
  topic_buf_t command_topic;
  get_command_topic(command_topic);

  // We don't need to specify payload_on/payload_off since HA defaults to "ON"/"OFF"
  auto doc = stack_json(node("command_topic", command_topic),           //
                        node_if("icon", config_.icon),                  //
                        node_if("device_class", config_.device_class),  //
                        node_if("entity_category", config_.entity_category));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

void Switch::handle_command(std::string_view payload) {
  if (payload == "ON") {
    turn_on();
  } else if (payload == "OFF") {
    turn_off();
  } else if (payload == "TOGGLE") {
    toggle();
  }
}

void Switch::turn_on() {
  set_state({true});
}

void Switch::turn_off() {
  set_state({false});
}

void Switch::toggle() {
  set_state({!state().is_on});
}

void Switch::on_change() {
  // Fire the user-provided callback whenever the state updates
  if (config_.on_change) {
    config_.on_change(ctx_, *this);
  }
}

}  // namespace HAPPY::Entities