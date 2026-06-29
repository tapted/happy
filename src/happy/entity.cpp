#include "happy/entity.hpp"

#include <algorithm>
#include <cstdio>
#include <format>
#include <ranges>

#include "espbase/json.h"
#include "espbase/nvs_store.hpp"

namespace HAPPY {

void Entity::initialize_base_topics(bool expects_commands) {
  discovery_topic_ =
      std::format("homeassistant/{}/{}/{}/config", domain_, device_.get_identifier(), object_id_);

  state_topic_ = std::format("{}/state", device_.get_identifier());

  if (expects_commands) {
    command_topic_ = std::format("{}/{}/set", device_.get_identifier(), object_id_);
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
  builder.set("name", name_);
  builder.set("unique_id", std::format("{}_{}", device_.get_identifier(), object_id_).c_str());
  builder.set("state_topic", state_topic_);

  // Inject the physical device grouping data
  device_.inject_into(builder);
}

}  // namespace HAPPY