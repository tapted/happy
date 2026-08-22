#include "happy/entities/text.hpp"

#include <algorithm>

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

bool Text::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;  // Max 32 entries.
  topic_buf_t command_topic;
  get_command_topic(command_topic);

  auto doc = stack_json(node("command_topic", command_topic),  //
                        node_if("icon", config_.icon),         //
                        node_if("entity_category", config_.entity_category));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

std::string Text::get_state_payload() {
  return state().value;
}

void Text::handle_command(std::string_view payload) {
  auto state = this->state();
  std::ranges::fill(state.value, '\0');

  // Truncate the payload view to fit the buffer (leaving room for the null terminator)
  size_t copy_len = std::min(payload.size(), sizeof(state.value) - 1);
  std::ranges::copy(payload.substr(0, copy_len), state.value);

  state.value[copy_len] = '\0';
  set_state(state);
}

}  // namespace HAPPY::Entities