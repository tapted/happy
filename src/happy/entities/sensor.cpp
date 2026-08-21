#include "happy/entities/sensor.hpp"

#include <esp_log.h>

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

bool Sensor::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;  // Max 32 entries.
  topic_buf_t state_topic;
  get_state_topic(state_topic);

  auto doc = stack_json(node(path("state_topic"), state_topic),
                        node_if(path("device_class"), config_.device_class),
                        node_if(path("unit_of_measurement"), config_.unit_of_measurement),
                        node_if(path("icon"), config_.icon),
                        node_if(path("entity_category"), config_.entity_category));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

}  // namespace HAPPY::Entities