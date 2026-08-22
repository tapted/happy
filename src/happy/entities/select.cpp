
#include "happy/entities/select.hpp"

#include <esp_log.h>

#include "espbase/stack_json/buffer.hpp"
#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

bool Select::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;  // Max 32 entries.
  topic_buf_t command_topic;
  get_command_topic(command_topic);

  auto doc = stack_json(node("command_topic", command_topic),  //
                        node_if("icon", config_.icon),         //
                        node_if("entity_category", config_.entity_category),
                        node("options", span_array(config_.options)));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

bool Select::get_state_payload(sjson::Buffer& buffer) {
  return buffer.write(get_selected());  // Select state is just the plain text string
}

void Select::handle_command(std::string_view payload) {
  // Validate the incoming payload against our allowed options
  for (size_t i = 0; i < config_.options.size(); ++i) {
    if (payload == config_.options[i]) {
      set_state({.selected_option_index_ = i});
      break;
    }
  }
}

void Select::on_change() {
  const size_t new_index = state().selected_option_index_;
  if (new_index != 0 && new_index >= config_.options.size()) {
    set_state({.selected_option_index_ = 0});  // Reset to a valid index if out of bounds
    return;                                    // Expect a recursive call.
  }

  if (config_.on_update) config_.on_update(on_update_ctx, *this);
}
}  // namespace HAPPY::Entities