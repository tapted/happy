#include "happy/entities/lazy_sensor.hpp"

#include <cmath>

#include "espbase/stack_json/buffer.hpp"
#include "espbase/stack_json/printer.hpp"

namespace HAPPY::Entities {

size_t format_tenths(sjson::Buffer& buffer, const int16_t& val) {
  int abs_val = std::abs(val);
  return sjson::Printer::printx(buffer, "%s%d.%d", (val < 0) ? "-" : "", abs_val / 10,
                                abs_val % 10);
}

void LazySensor::refresh_and_maybe_publish() {
  state_->refresh();
  if (state_->needs_publish()) {
    request_publish();  // Eventually calls get_payload() via the standard Entity pipeline
  }
}

}  // namespace HAPPY::Entities