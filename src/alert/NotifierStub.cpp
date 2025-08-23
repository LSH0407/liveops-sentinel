#include "Notifier.h"
#include <spdlog/spdlog.h>

namespace alert {

bool Notifier::sendDiscord(const std::string& content) {
  spdlog::info("[STUB] Discord webhook skipped: {}", content);
  return true;
}

bool Notifier::isReady() const { return true; }

} // namespace alert