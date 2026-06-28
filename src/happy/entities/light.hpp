#pragma once

#include <cstdint>
#include <functional>

#include "happy/entity.hpp"

namespace HAPPY::Entities {

struct RgbColor {
  uint32_t r, g, b;
};

// The POD state that gets written to flash in a single atomic block
struct LightState {
  bool is_on = false;
  uint8_t brightness = 255;
  uint8_t r = 255, g = 255, b = 255;
};

// A Light entity that supports ON/OFF, brightness, and RGB color.
// E.g.,
// ```cpp
// static void on_light_update(const HAPPY::Entities::Light& light) {
//   auto& strip = HAL::LedStrip::default_instance();
//   auto [r, g, b] = light.scaled_rgb();
//   strip.set_pixel(0, r, g, b);
//   strip.refresh();
// }
//
// HAPPY::Entities::Light onboard_led(dongley_device, "status_led", "Onboard LED",
//                                    {
//                                        .supports_rgb = true,
//                                        .on_update = on_light_update,
//                                    });
// ```
class Light : public PersistentEntity<Light, LightState> {
 public:
  struct Config {
    const char* icon = "mdi:led-strip-variant";
    bool supports_rgb = true;
    std::function<void(const Light&)> on_update = nullptr;
  };

  constexpr Light(Device& device, std::string_view object_id, std::string_view name, Config config)
      : PersistentEntity<Light, LightState>(device, "light", object_id, name),
        config_(std::move(config)) {}

  // --- State Accessors ---
  bool is_on() const { return state_.is_on; }
  uint8_t brightness() const { return state_.brightness; }
  RgbColor raw_rgb() const { return {state_.r, state_.g, state_.b}; }

  // Computes the final RGB output. If OFF, returns {0,0,0}.
  RgbColor scaled_rgb() const {
    if (!state_.is_on) return {0, 0, 0};
    return {
        static_cast<uint32_t>((state_.r * state_.brightness) / 255),
        static_cast<uint32_t>((state_.g * state_.brightness) / 255),
        static_cast<uint32_t>((state_.b * state_.brightness) / 255),
    };
  }
  std::string get_discovery_payload() const override;
  std::string get_state_payload() const override;

  void initialize_topics() override {
    bool loaded_from_nvs = initialize_base_topics(true);
    if (loaded_from_nvs && config_.on_update) config_.on_update(*this);
  }
  
  void handle_command(const std::string_view payload) override;

 private:
  Config config_;
  bool is_on_ = false;
  uint8_t brightness_ = 255;
  uint8_t r_ = 255, g_ = 255, b_ = 255;
};

}  // namespace HAPPY::Entities