#include "happy/entities/text.hpp"

#include <algorithm>
#include <cstring>

#include "espbase/stack_json/json.hpp"

namespace HAPPY::Entities {

template class TextT<TextBase::kDefaultBufferSize>;

TextBase::TextBase(Device& device, const char* object_id, const char* name, Config config,
                   std::span<char> buffer, void* ctx)
    : Entity(device,
             {
                 .domain = "text",
                 .object_id = object_id,
                 .name = name,
                 .retain_state = true,
                 .expects_commands = true,
             }),
      config_(config),
      buffer_(buffer),
      ctx_(ctx) {
}

bool TextBase::get_discovery_payload(sjson::Buffer& buffer) {
  sjson::StackBuilder<32> builder;  // Max 32 entries.
  topic_buf_t command_topic;
  get_command_topic(command_topic);

  // optimistic:true because HA won't update to show sanitised payloads and will leave the text
  // field in a dirty state when the sanitised payload is unchanged. Optimistic means the text field
  // always gets dirty and will return to the sanitised payload when it changes.
  auto doc = stack_json(node("command_topic", command_topic),  //
                        node("optimistic", true),              //
                        node_if("icon", config_.icon),         //
                        node_if("entity_category", config_.entity_category));

  builder.add(doc);
  return this->emit_with_base_config(buffer, builder);
}

size_t TextBase::get_state_payload(sjson::Buffer& buffer) {
  return buffer.write(buffer_.data());
}

void TextBase::handle_command(std::string_view payload) {
  set_value(payload);
}

void TextBase::set_value(std::string_view new_value) {
  // Truncate the payload view to fit the buffer (leaving room for the null terminator)
  size_t copy_len = std::min(new_value.size(), buffer_.size() - 1);
  std::string_view truncated_new(new_value.data(), copy_len);

  // Prevent unnecessary NVS wear if the value hasn't changed
  if (std::string_view(buffer_.data()) == truncated_new) {
    return;
  }

  std::ranges::fill(buffer_, '\0');
  std::ranges::copy(truncated_new, buffer_.data());

  this->save_nvs_blob(buffer_.data(), buffer_.size());
  this->request_publish();
  this->on_change();
}

void TextBase::load() {
  // Mirrors PersistentEntity::load(), but here to avoid template bloat.
  this->load_nvs_blob(buffer_.data(), buffer_.size());
  this->on_change();
}

void TextBase::on_change() {
  if (config_.on_update) {
    config_.on_update(ctx_, *this);
  }
}

}  // namespace HAPPY::Entities