#pragma once
#include <memory>
#include <iostream>

class Dashboard;

class App {
public:
  App();
  ~App();
  bool init();
  void run();
private:
  bool running_{true};
  std::unique_ptr<Dashboard> ui_;
};
