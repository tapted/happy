#include "happy/entities/time.hpp"

#include <charconv>

#include "espbase/json.hpp"

namespace HAPPY::Entities {

std::string Time::get_discovery_payload() const {
  JsonDocument doc;
  JsonObjectBuilder builder(doc.get());
  this->inject_base_config(builder);

  topic_buf_t command_topic;
  get_command_topic(command_topic);
  builder.set("command_topic", (const char*)command_topic);

  if (config_.icon) builder.set("icon", config_.icon);

  return doc.to_string();
}

std::string Time::get_state_payload() const {
  char buffer[12];  // "HH:MM:SS" + null terminator
  std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour(), minute(), second());
  return std::string(buffer);
}

void Time::handle_command(std::string_view payload) {
  auto state = this->state();
  // Payload is "HH:MM" or "HH:MM:SS"
  if (payload.length() >= 5) {
    std::from_chars(payload.data(), payload.data() + 2, state.hour_);
    std::from_chars(payload.data() + 3, payload.data() + 5, state.minute_);

    if (payload.length() >= 8) {
      std::from_chars(payload.data() + 6, payload.data() + 8, state.second_);
    } else {
      state.second_ = 0;
    }
    set_state(state);
  }
}

}  // namespace HAPPY::Entities