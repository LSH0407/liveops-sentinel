#pragma once
#include <string>
#include "ipc/Json.h"

struct Thresholds { 
  int rttMs{80}; 
  double lossPct{2.0}; 
  int holdSec{5}; 
};

class Sentinel {
public:
  void setWebhook(std::string url);
  void setThresholds(Thresholds t);
  void tickAndEmitMetrics(); // 1s 주기 호출, IpcLoop::send로 metrics push
  json runPreflight();       // 체크리스트
  // OBS는 Stub 기준 안전하게 false 반환 허용
  bool startStream(); 
  bool stopStream(); 
  bool setScene(const std::string& name);
private:
  Thresholds th_;
  std::string webhook_;
  // TODO: Probe/ReportWriter/Notifier 등 기존 구성품 연결
};
