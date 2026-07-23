#include "happy/entity.hpp"

#include <algorithm>
#include <cstdio>
#include <ranges>

#include "espbase/json.h"
#include "espbase/nvs_store.hpp"
#include "happy/device.hpp"

namespace HAPPY {

Entity::Entity(Device& device, std::string_view domain, std::string_view object_id,
               std::string_view name)
    : device_(device), domain_(domain), object_id_(object_id), name_(name) {
  // Register the entity with the device upon construction. This means the constructors can't be
  // constexpr/constinit. But the registration structs do not require a heap allocation, so this
  // is still safe for static initialization.
  device_.register_entity(this);
}

void Entity::request_publish() {
  pending_flags_.fetch_or(FLAG_STATE, std::memory_order_release);
  device_.poke();
}

void Entity::request_discovery() {
  uint8_t flags = FLAG_DISCOVERY;
  if (!command_topic_.empty()) {
    flags |= FLAG_SUBSCRIBE;
  }
  pending_flags_.fetch_or(flags, std::memory_order_release);
  device_.poke();
}

void Entity::initialize_base_topics(bool expects_commands) {
  char buf[128];
  snprintf(buf, sizeof(buf), "homeassistant/%.*s/%s/%.*s/config",
           static_cast<int>(domain_.length()), domain_.data(), device_.get_identifier(),
           static_cast<int>(object_id_.length()), object_id_.data());
  discovery_topic_ = buf;

  snprintf(buf, sizeof(buf), "%s/%.*s/state", device_.get_identifier(),
           static_cast<int>(object_id_.length()), object_id_.data());
  state_topic_ = buf;

  if (expects_commands) {
    snprintf(buf, sizeof(buf), "%s/%.*s/set", device_.get_identifier(),
             static_cast<int>(object_id_.length()), object_id_.data());
    command_topic_ = buf;
  }
}

bool Entity::load_nvs_blob(void* dest, size_t size) const {
  char ns[16]{};
  char key[16]{};

  std::snprintf(ns, sizeof(ns), "ha_%.*s", static_cast<int>(domain_.length()), domain_.data());

  auto key_view = object_id_ | std::views::take(15);
  std::ranges::copy(key_view, key);

  auto store_res = NvsStore::open(ns, NVS_READONLY);
  return store_res && store_res->get_raw_blob(NvsStore::Key(key), dest, size);
}

void Entity::save_nvs_blob(const void* src, size_t size) const {
  char ns[16]{};
  char key[16]{};

  std::snprintf(ns, sizeof(ns), "ha_%.*s", static_cast<int>(domain_.length()), domain_.data());

  auto key_view = object_id_ | std::views::take(15);
  std::ranges::copy(key_view, key);

  auto store_res = NvsStore::open(ns, NVS_READWRITE);
  if (store_res) {
    store_res->set_raw_blob(NvsStore::Key(key), src, size).log_error("Entity", key);
    store_res->commit().log_error("Entity", key);
    ESP_LOGD("Entity", "Saved state to NVS: %s/%s (%zu bytes)", ns, key, size);
  }
}

void Entity::inject_base_config(JsonObjectBuilder& builder) const {
  char buf[128];
  snprintf(buf, sizeof(buf), "%s_%.*s", device_.get_identifier(),
           static_cast<int>(object_id_.length()), object_id_.data());
  const char* unique_id = buf;

  builder.set("name", name_);
  builder.set("unique_id", unique_id);
  builder.set("state_topic", state_topic_);

  // Inject the physical device grouping data
  device_.inject_into(builder);
}

}  // namespace HAPPY