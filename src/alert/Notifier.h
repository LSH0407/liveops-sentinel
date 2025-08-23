#pragma once
#include <string>

namespace alert {

class Notifier {
public:
  bool sendDiscord(const std::string& content);
  bool isReady() const;
};

} // namespace alert