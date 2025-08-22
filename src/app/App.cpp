#include "App.h"
#include "../ui/Dashboard.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <chrono>
#include <thread>

App::App() {}
App::~App(){
  // Console application cleanup
}

bool App::init(){
  spdlog::info("LiveOps Sentinel Console Application Starting...");
  ui_ = std::make_unique<Dashboard>();
  return true;
}

void App::run(){
  spdlog::info("LiveOps Sentinel Console Application Running...");
  std::cout << "LiveOps Sentinel Console Application" << std::endl;
  std::cout << "Press Ctrl+C to exit" << std::endl;
  
  while (running_){
    // Console application main loop
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
