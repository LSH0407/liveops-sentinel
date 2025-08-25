#pragma once
#include <functional>
#include <string>
#include <atomic>
#include "ipc/Json.h"

class IpcLoop {
public:
  using Handler = std::function<void(const json& req)>;
  explicit IpcLoop(Handler h);
  void run();
  static void send(const json& msg);
private:
  Handler handler_;
  std::atomic<bool> running_{true};
};
