#include "happy/entities/status.hpp"

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

void StaticStatus::set_state(const char* new_state) {
  current_state_ = new_state;
  request_publish();
}

size_t StaticStatus::get_state_payload(sjson::Buffer& buffer) {
  return buffer.write(current_state_ ? current_state_ : "unknown");
}

bool StaticStatus::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;

  // No command_topic needed since expects_commands is false
  auto doc = stack_json(node_if("icon", config_.icon),
                        node_if("entity_category", config_.entity_category));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

}  // namespace HAPPY::Entities