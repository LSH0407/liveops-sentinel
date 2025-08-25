#include "ipc/IpcLoop.h"
#include "core/Sentinel.h"
#include <thread>

int main(){
  Sentinel s;
  IpcLoop loop([&](const json& req){
    std::string cmd = req.value("cmd", "");
    if (cmd=="ping") 
      IpcLoop::send({{"event","pong"}});
    else if (cmd=="set_webhook") 
      s.setWebhook(req.value("url",""));
    else if (cmd=="set_thresholds") 
      s.setThresholds({req.value("rttMs",80), req.value("lossPct",2.0), req.value("holdSec",5)});
    else if (cmd=="get_metrics") 
      s.tickAndEmitMetrics();
    else if (cmd=="preflight") 
      IpcLoop::send(s.runPreflight());
    else if (cmd=="start_stream") 
      IpcLoop::send({{"event","log"},{"level","info"},{"msg", s.startStream()?"ok":"stub"}});
    else if (cmd=="stop_stream")  
      IpcLoop::send({{"event","log"},{"level","info"},{"msg", s.stopStream() ?"ok":"stub"}});
    else if (cmd=="set_scene")    
      IpcLoop::send({{"event","log"},{"level","info"},{"msg", s.setScene(req.value("name",""))?"ok":"stub"}});
    else 
      IpcLoop::send({{"event","log"},{"level","warn"},{"msg","unknown_cmd"}});
  });
  
  // metrics 주기 푸시: 간단히 1초 주기 호출(데모)
  std::thread t([&](){ 
    while(true){ 
      s.tickAndEmitMetrics(); 
      std::this_thread::sleep_for(std::chrono::seconds(1)); 
    } 
  });
  
  loop.run(); // stdin 루프
  t.join();
  return 0;
}
