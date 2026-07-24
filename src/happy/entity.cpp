#include "happy/entity.hpp"

#include <cstdio>

#include "espbase/json.hpp"
#include "espbase/nvs_store.hpp"
#include "happy/device.hpp"

namespace HAPPY {

Entity::Entity(Device& device, const char* domain, const char* object_id, const char* name,
               bool expects_commands)
    : device_(device), domain_(domain), object_id_(object_id), name_(name) {
  if (expects_commands) {
    pending_flags_.fetch_or(EXPECTS_COMMANDS, std::memory_order_relaxed);
  }
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
  if (expects_commands()) {
    flags |= FLAG_SUBSCRIBE;
  }
  pending_flags_.fetch_or(flags, std::memory_order_release);
  device_.poke();
}

void Entity::get_discovery_topic(topic_buf_t& buf) const {
  snprintf(buf, sizeof(buf), "homeassistant/%s/%s/%s/config", domain_, device_.get_identifier(),
           object_id_);
}

void Entity::get_state_topic(topic_buf_t& buf) const {
  snprintf(buf, sizeof(buf), "%s/%s/state", device_.get_identifier(), object_id_);
}

void Entity::get_command_topic(topic_buf_t& buf) const {
  if (!expects_commands()) {
    buf[0] = '\0';
    return;
  }
  snprintf(buf, sizeof(buf), "%s/%s/set", device_.get_identifier(), object_id_);
}

bool Entity::load_nvs_blob(void* dest, size_t size) const {
  char ns[16]{};
  char key[16]{};

  snprintf(ns, sizeof(ns), "ha_%s", domain_);
  snprintf(key, sizeof(key), "%s", object_id_);

  auto store_res = NvsStore::open(ns, NVS_READONLY);
  return store_res && store_res->get_raw_blob(NvsStore::Key(key), dest, size);
}

void Entity::save_nvs_blob(const void* src, size_t size) const {
  char ns[16]{};
  char key[16]{};

  snprintf(ns, sizeof(ns), "ha_%s", domain_);
  snprintf(key, sizeof(key), "%s", object_id_);

  auto store_res = NvsStore::open(ns, NVS_READWRITE);
  if (store_res) {
    store_res->set_raw_blob(NvsStore::Key(key), src, size).log_error("Entity", key);
    store_res->commit().log_error("Entity", key);
    ESP_LOGD("Entity", "Saved state to NVS: %s/%s (%zu bytes)", ns, key, size);
  }
}

void Entity::inject_base_config(JsonObjectBuilder& builder) const {
  char buf[128];
  snprintf(buf, sizeof(buf), "%s_%s", device_.get_identifier(), object_id_);
  const char* unique_id = buf;

  builder.set("name", name_);
  builder.set("unique_id", unique_id);

  get_state_topic(buf);
  builder.set("state_topic", (const char*)buf);

  // Inject the physical device grouping data
  device_.inject_into(builder);
}

}  // namespace HAPPY