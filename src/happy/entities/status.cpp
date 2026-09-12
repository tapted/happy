#include "happy/entities/status.hpp"

#include "espbase/stack_json/json.hpp"
#include "happy/device.hpp"

namespace HAPPY::Entities {

StaticStatus::StaticStatus(Device& device, const char* object_id, const char* name,
                           const char* initial_state, Config config)
    : Entity(device,
             {
                 .domain = "sensor",
                 .object_id = object_id,
                 .name = name,
                 .retain_state = true,
                 .expects_commands = false,
             }),
      config_(std::move(config)),
      current_state_(initial_state) {
}

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

LastWillStatus::LastWillStatus(Device& device)
    : StaticStatus(device, HAPPY::Device::kStatusIdentifier, "Device Status", "online",
                   {.icon = "mdi:check-network"}) {
}
}  // namespace HAPPY::Entities