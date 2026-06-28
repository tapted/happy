#include "happy/entity.hpp"

#include <format>

#include "espbase/json.h"

namespace HAPPY {
  
void Entity::initialize_base_topics(bool expects_commands) {
  discovery_topic_ =
      std::format("homeassistant/{}/{}/{}/config", domain_, device_.get_identifier(), object_id_);

  state_topic_ = std::format("{}/state", device_.get_identifier());

  if (expects_commands) {
    command_topic_ = std::format("{}/{}/set", device_.get_identifier(), object_id_);
  }
}

void Entity::inject_base_config(JsonObjectBuilder& builder) const {
  builder.set("name", name_);
  builder.set("unique_id", std::format("{}_{}", device_.get_identifier(), object_id_).c_str());
  builder.set("state_topic", state_topic_);

  // Inject the physical device grouping data
  device_.inject_into(builder);
}

}  // namespace HAPPY