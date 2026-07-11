#include "happy/entities/text.hpp"

#include <algorithm>

#include "espbase/json.h"


namespace HAPPY::Entities {

std::string Text::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  builder.set("command_topic", command_topic_.c_str());
  if (config_.icon) builder.set("icon", config_.icon);
  if (config_.entity_category) builder.set("entity_category", config_.entity_category);

  return doc.to_string();
}

std::string Text::get_state_payload() const {
  return state().value;
}

void Text::initialize_topics() {
  initialize_base_topics(true);
}

void Text::handle_command(std::string_view payload) {
  auto state = this->state();
  std::ranges::fill(state.value, '\0');
  std::ranges::copy(payload, state.value);
  state.value[sizeof(state.value) - 1] = '\0';
  set_state(state);
}

}  // namespace HAPPY::Entities