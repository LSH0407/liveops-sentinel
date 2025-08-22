#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>

struct BenchConfig {
    int durationSec{30};
    std::string proto{"udp"};
    double headroom{0.75};
    int minKbps{800};
    int maxKbps{15000};
    int packetSize{1024};
    int packetsPerSec{1000};
    bool collectSystemMetrics{true};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BenchConfig, durationSec, proto, headroom, minKbps, maxKbps, packetSize, packetsPerSec, collectSystemMetrics)
};

struct ThresholdConfig {
    double rttMs{80.0};
    double lossPct{2.0};
    double droppedFramesRatio{0.02};
    double encodingLagMs{25.0};
    double renderLagMs{20.0};
    int holdSec{5};
    int cooldownSec{60};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ThresholdConfig, rttMs, lossPct, droppedFramesRatio, encodingLagMs, renderLagMs, holdSec, cooldownSec)
};

struct ReportConfig {
    bool enable{true};
    int flushIntervalSec{10};
    std::string dir{"reports"};
    int maxFileSizeMB{25};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReportConfig, enable, flushIntervalSec, dir, maxFileSizeMB)
};

struct ObsConfig {
    std::string host{"127.0.0.1"};
    int port{4455};
    std::string password{""};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ObsConfig, host, port, password)
};

struct PreflightConfig {
    std::vector<std::string> ueProcessHints{"UnrealEditor.exe", "UE4Editor.exe"};
    std::string ndiInputKindHint{"ndi"};
    int diskMinGB{10};
    bool warnIfWifi{true};
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PreflightConfig, ueProcessHints, ndiInputKindHint, diskMinGB, warnIfWifi)
};

struct Config {
  // Probe 설정
  std::string probeHost="127.0.0.1";
  int probePort=50051;
  int probeRateHz=20;
  
  // 알림 설정
  std::string discordWebhook="";
  bool webhookConfigured{false};
  std::string discordUsername="LiveOps Sentinel";
  double rttThreshold=100.0;
  double lossThreshold=5.0;
  bool enableDiscord=true;
  
  // 벤치마크 설정
  BenchConfig bench;
  
  // 임계치 설정
  ThresholdConfig thresholds;
  
  // 리포트 설정
  ReportConfig report;
  
  // OBS 설정
  ObsConfig obs;
  
  // Pre-flight 설정
  PreflightConfig preflight;
  
  // 모니터링할 프로세스
  std::vector<std::string> monitoredProcesses={"obs64.exe", "UnrealEditor.exe"};
  
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, probeHost, probePort, probeRateHz, 
                                discordWebhook, webhookConfigured, discordUsername, rttThreshold, lossThreshold, 
                                enableDiscord, bench, thresholds, report, obs, preflight, monitoredProcesses)
};
bool LoadConfig(const std::string& path, Config& out);
bool SaveConfig(const std::string& path, const Config& in);

// 사용자별 설정 경로/로드/세이브 (레포 외부)
std::filesystem::path GetUserConfigPath();
bool LoadUserConfig(Config& out);
bool SaveUserConfig(const Config& in);

// UI 표시용 웹후크 마스킹
inline std::string MaskWebhook(const std::string& url){
  if (url.empty()) return "(not set)";
  std::string s=url; if(s.size()>32){ s.replace(8, s.size()-16, "****"); }
  return s;
}
