#include "happy/entities/day_mask.hpp"

#include <string>

namespace HAPPY::Entities {

namespace {

constexpr std::string parse_day_mask(std::string_view input) {
  constexpr std::string_view target_days = "SMTWTFS";
  std::string result = "_______";
  size_t mask_idx = 0;

  for (char c : input) {
    if (mask_idx >= 7) {
      break;
    }

    // Convert to uppercase inline for constexpr support
    char upper_c = (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;

    if (upper_c == '_' || upper_c == ' ') {
      // Unconditionally consume and lock the next available slot
      mask_idx++;
    } else if (upper_c == 'S' || upper_c == 'M' || upper_c == 'T' || upper_c == 'W' ||
               upper_c == 'F') {
      // Search forward for the next matching day character
      size_t match_idx = target_days.find(upper_c, mask_idx);

      if (match_idx != std::string_view::npos) {
        result[match_idx] = upper_c;
        mask_idx = match_idx + 1;  // Advance past the matched slot
      }
    }
    // All other characters are completely ignored
  }

  return result;
}

}  // anonymous namespace

void DayMask::handle_command(std::string_view payload) {
  // Intercept, parse, and pass the cleaned string to TextBase
  std::string parsed = parse_day_mask(payload);
  set_value(parsed);
}

uint8_t DayMask::get_bitmask() const {
  std::string_view current_mask = get_value();
  uint8_t bitmask = 0;

  // Safety check to ensure the string is fully formed
  if (current_mask.length() >= 7) {
    for (int i = 0; i < 7; ++i) {
      if (current_mask[i] != '_') {
        bitmask |= (1 << i);
      }
    }
  }

  return bitmask;
}
}  // namespace HAPPY::Entities