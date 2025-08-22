#pragma once
#include <SDL.h>
#include <memory>

class Dashboard;

class App {
public:
  App();
  ~App();
  bool init();
  void run();
private:
  SDL_Window* window_{};
  SDL_GLContext glctx_{};
  bool running_{true};
  std::unique_ptr<Dashboard> ui_;
};
