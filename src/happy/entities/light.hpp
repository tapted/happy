#pragma once

#include <cstdint>
#include <functional>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct RgbColor {
  uint32_t r, g, b;
};

class Light : public Entity {
 public:
  struct Config {
    const char* icon = "mdi:led-strip-variant";
    bool supports_rgb = true;
    std::function<void(const Light&)> on_update = nullptr;
  };

  constexpr Light(Device& device, std::string_view object_id, std::string_view name, Config config)
      : Entity(device, "light", object_id, name), config_(std::move(config)) {}

  // --- State Accessors ---
  bool is_on() const { return is_on_; }
  uint8_t brightness() const { return brightness_; }
  RgbColor raw_rgb() const { return {r_, g_, b_}; }

  // Computes the final RGB output. If OFF, returns {0,0,0}.
  RgbColor scaled_rgb() const {
    if (!is_on_) return {0, 0, 0};
    return {
        static_cast<uint32_t>((r_ * brightness_) / 255),
        static_cast<uint32_t>((g_ * brightness_) / 255),
        static_cast<uint32_t>((b_ * brightness_) / 255),
    };
  }
  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;

  void initialize_topics() override { initialize_base_topics(true); }
  void handle_command(const unique_cjson& root) override;

 private:
  Config config_;
  bool is_on_ = false;
  uint8_t brightness_ = 255;
  uint8_t r_ = 255, g_ = 255, b_ = 255;
};

}  // namespace HAPPY::Entities