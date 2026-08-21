#include "happy/device.hpp"

#if __has_include(<esp_app_desc.h>)
#include <esp_app_desc.h>
#else
struct esp_app_desc_t {
  const char* version;
};
esp_app_desc_t* esp_app_get_description() {
  static esp_app_desc_t desc = {
      .version = "unknown",
  };
  return &desc;
}
#endif
#include <esp_log.h>

#include "espbase/json.hpp"
#include "espbase/mac_address.hpp"
#include "espbase/stack_json/json.hpp"
#include "happy/entity.hpp"

namespace HAPPY {

const char* Device::get_mac_chars(char (&buf)[16]) const {
  const MacAddress& m = MacAddress::mine();
  std::snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return buf + (12 - config_.append_mac_chars);
}

const char* Device::get_unique_id(char (&buf)[128], const char* object_id, char object_sep) const {
  if (config_.append_mac_chars == 0) {
    std::snprintf(buf, sizeof(buf), "%s%c%s", config_.identifiers, object_sep, object_id);
    return buf;
  }

  char mac_buf[16];
  const char* mac_ptr = get_mac_chars(mac_buf);
  std::snprintf(buf, sizeof(buf), "%s_%s%c%s", config_.identifiers, mac_ptr, object_sep, object_id);
  return buf;
}

// Injects the HA "device" grouping block into an existing json.h builder
void Device::inject_into(JsonObjectBuilder& builder) const {
  char identifier_buf[64];
  char name_buf[64];

  const char* identifier = config_.identifiers;
  const char* name = config_.name;

  if (config_.append_mac_chars > 0) {
    char buf[16];
    const char* mac_ptr = get_mac_chars(buf);

    std::snprintf(identifier_buf, sizeof(identifier_buf), "%s_%s", config_.identifiers, mac_ptr);
    std::snprintf(name_buf, sizeof(name_buf), "%s %s", mac_ptr, config_.name);
    identifier = identifier_buf;
    name = name_buf;
  }

  builder.with_object("device", [this, identifier, name](auto& dev) {
    dev.with_array("identifiers", [identifier](auto& arr) { arr.push(identifier); });
    dev.set("name", name);

    if (config_.manufacturer) dev.set("manufacturer", config_.manufacturer);
    if (config_.model) dev.set("model", config_.model);

    const char* sw_version =
        config_.sw_version ? config_.sw_version : esp_app_get_description()->version;
    dev.set("sw_version", sw_version);
  });
}

bool Device::emit_with(sjson::Buffer& buffer, sjson::Builder& builder) const {
  char identifier_buf[64];
  char name_buf[64];

  const char* identifier = config_.identifiers;
  const char* name = config_.name;

  if (config_.append_mac_chars > 0) {
    char buf[16];
    const char* mac_ptr = get_mac_chars(buf);

    std::snprintf(identifier_buf, sizeof(identifier_buf), "%s_%s", config_.identifiers, mac_ptr);
    std::snprintf(name_buf, sizeof(name_buf), "%s %s", mac_ptr, config_.name);
    identifier = identifier_buf;
    name = name_buf;
  }

  const char* sw_ver = config_.sw_version ? config_.sw_version : esp_app_get_description()->version;
  auto device = path("device");
  auto doc = stack_json(node(device("identifiers"), stack_array(identifier)),   //
                        node(device("name"), name),                             //
                        node_if(device("manufacturer"), config_.manufacturer),  //
                        node_if(device("model"), config_.model),                //
                        node(device("sw_version"), sw_ver));
  builder.add(doc);
  return builder.emit(buffer);
}

void Device::register_entity(HAPPY::Entity* entity) {
  entities_.push_back(entity);
  // Note: entity might not be fully initialized yet.
}

void Device::republish_all() {
  for (Entity& entity : entities_) {
    publish(entity);
  }
}

int Device::publish(Entity& entity) {
  entity.request_publish();
  return 0;
}

void Device::load() {
  if (loaded_) return;
  loaded_ = true;
  for (Entity& entity : entities_) {
    entity.load();
  }
}

void Device::begin() {
  load();
}

void Device::dispatch_command(std::string_view topic, std::string_view payload) const {
  topic_buf_t command_topic;
  for (Entity& entity : entities_) {
    if (!entity.expects_commands()) continue;
    entity.get_command_topic(command_topic);
    if (std::string_view(command_topic) == topic) {
      entity.handle_command(payload);
      return;  // Stop searching once routed
    }
  }
}

}  // namespace HAPPY