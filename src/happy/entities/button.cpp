#include "happy/entities/button.hpp"

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

bool Button::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;  // Max 32 entries.
  topic_buf_t command_topic;
  get_command_topic(command_topic);

  auto doc = stack_json(node(path("command_topic"), command_topic),  //
                        node_if(path("icon"), config_.icon),         //
                        node_if(path("device_class"), config_.device_class),
                        node_if(path("entity_category"), config_.entity_category));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

void Button::handle_command(std::string_view payload) {
  if (payload == "PRESS" && config_.on_press) {
    config_.on_press(ctx_, *this);
  }
}

}  // namespace HAPPY::Entities