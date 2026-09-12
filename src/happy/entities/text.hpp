#pragma once

#include <span>
#include <string_view>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

class TextBase : public Entity {
 public:
  static constexpr size_t kDefaultBufferSize = 64;
  struct Config {
    const char* icon = "mdi:form-textbox";
    const char* entity_category = "config";
    void (*on_update)(void* ctx, const TextBase&) = nullptr;
  };

  TextBase(Device& device, const char* object_id, const char* name, Config config,
           std::span<char> buffer, void* ctx = nullptr);

  void set_value(std::string_view new_value);
  std::string_view get_value() const { return std::string_view(buffer_.data()); }

  bool get_discovery_payload(sjson::Buffer& buffer) override;
  size_t get_state_payload(sjson::Buffer& buffer) override;
  void handle_command(std::string_view payload) override;
  void load() override;

 private:
  void on_change();

  Config config_;
  std::span<char> buffer_;
  void* ctx_ = nullptr;
};

template <size_t MaxLen = TextBase::kDefaultBufferSize>
class TextT : public TextBase {
 public:
  TextT(Device& device, const char* object_id, const char* name, TextBase::Config config,
        const char* default_val = "", void* ctx = nullptr)
      : TextBase(device, object_id, name, config, storage_, ctx) {
    snprintf(storage_, MaxLen, "%s", default_val);  // Seed the storage. No NVS writes.
  }

 private:
  char storage_[MaxLen]{};
};

using Text = TextT<TextBase::kDefaultBufferSize>;
extern template class TextT<TextBase::kDefaultBufferSize>;

}  // namespace HAPPY::Entities