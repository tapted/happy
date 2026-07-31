#include "happy/entities/lazy_sensor.hpp"

#include <cmath>

namespace HAPPY::Entities {

std::string format_tenths(const int16_t& val) {
  char buf[16];
  int abs_val = std::abs(val);
  snprintf(buf, sizeof(buf), "%s%d.%d", (val < 0) ? "-" : "", abs_val / 10, abs_val % 10);
  return std::string(buf);
}

void LazySensor::refresh_and_maybe_publish() {
  state_->refresh();
  if (state_->needs_publish()) {
    request_publish();  // Eventually calls get_payload() via the standard Entity pipeline
  }
}

}  // namespace HAPPY::Entities