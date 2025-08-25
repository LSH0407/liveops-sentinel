#include "ipc/IpcLoop.h"
#include <iostream>

IpcLoop::IpcLoop(Handler h):handler_(std::move(h)){}

void IpcLoop::run(){
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;
    try { 
      json j = json::parse(line); 
      handler_(j); 
    }
    catch (...) { 
      json err={{"event","log"},{"level","error"},{"msg","bad_json"}}; 
      send(err); 
    }
  }
}

void IpcLoop::send(const json& msg){ 
  std::cout << msg.dump() << std::endl; 
}
