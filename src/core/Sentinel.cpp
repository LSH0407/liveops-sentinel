#include "Sentinel.h"
#include "ipc/IpcLoop.h"
#include "net/Probe.h"
#include "net/BandwidthTest.h"
#include "sys/ProcessMon.h"
#include "obs/ObsClient.h"
#include "notify/AlertManager.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <spdlog/spdlog.h>

void Sentinel::setWebhook(std::string url){ 
  webhook_=std::move(url); 
}

void Sentinel::setThresholds(Thresholds t){ 
  th_=t; 
}

void Sentinel::tickAndEmitMetrics(){
  static Probe networkProbe;
  static ProcessMonitor systemMonitor;
  static ObsClient obsClient;
  static AlertManager alertManager;
  static bool initialized = false;
  
  // 초기화 (한 번만)
  if (!initialized) {
    // 네트워크 프로브는 자동으로 메트릭을 수집합니다
    spdlog::info("네트워크 프로브 초기화 완료");
    
    // OBS 프로세스 모니터링 추가
    systemMonitor.addProcess("obs64");
    systemMonitor.addProcess("obs32");
    
    // OBS WebSocket 연결 시도
    if (obsClient.connect("ws://localhost:4444")) {
      spdlog::info("OBS WebSocket 연결 성공");
    } else {
      spdlog::warn("OBS WebSocket 연결 실패 - OBS가 실행 중이지 않거나 WebSocket이 비활성화됨");
    }
    
    // 알림 시스템 설정
    AlertThresholds thresholds;
    thresholds.rtt_ms_warning = 80.0;
    thresholds.rtt_ms_critical = 150.0;
    thresholds.loss_pct_warning = 2.0;
    thresholds.loss_pct_critical = 5.0;
    thresholds.cpu_pct_warning = 80.0;
    thresholds.cpu_pct_critical = 95.0;
    thresholds.gpu_pct_warning = 85.0;
    thresholds.gpu_pct_critical = 95.0;
    thresholds.dropped_ratio_warning = 0.03;
    thresholds.dropped_ratio_critical = 0.08;
    thresholds.hold_seconds = 5;
    
    alertManager.setThresholds(thresholds);
    alertManager.setAlertCallback([](const Alert& alert) {
      // 알림을 IPC로 전송
      json alert_msg = {
        {"event", "alert"},
        {"level", static_cast<int>(alert.level)},
        {"title", alert.title},
        {"message", alert.message},
        {"source", alert.source},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            alert.timestamp.time_since_epoch()).count()},
        {"metadata", alert.metadata}
      };
      IpcLoop::send(alert_msg);
    });
    
    initialized = true;
    spdlog::info("실제 모니터링 시스템 초기화 완료");
  }
  
  // 실제 시스템 메트릭 수집
  auto systemMetrics = systemMonitor.getSystemMetrics();
  
  // 네트워크 메트릭 수집
  auto network_metrics = networkProbe.getMetrics();
  double rtt = network_metrics["rtt_ms"];
  double loss = network_metrics["loss_pct"];
  
  // 실제 대역폭 측정 (최근 측정값 사용)
  static BandwidthTest bandwidthTest;
  static double last_measured_uplink_mbps = 8.0; // 기본값
  static std::chrono::steady_clock::time_point last_bandwidth_test = std::chrono::steady_clock::now();
  
  // 5분마다 대역폭 재측정
  auto now = std::chrono::steady_clock::now();
  if (now - last_bandwidth_test > std::chrono::minutes(5)) {
    bandwidthTest.runTestAsync("speed.cloudflare.com", 10, [&](const BandwidthResult& result) {
      if (result.success) {
        last_measured_uplink_mbps = result.upload_mbps;
        spdlog::info("대역폭 재측정 완료: {:.1f}Mbps", last_measured_uplink_mbps);
      }
    });
    last_bandwidth_test = now;
  }
  
  double uplink_kbps = last_measured_uplink_mbps * 1000.0; // Mbps to kbps
  
  // OBS 통계 수집
  auto obsStats = obsClient.getStats();
  
  json m = {
    {"event","metrics"},
    {"ts", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()},
    {"rtt_ms", rtt},
    {"loss_pct", loss},
    {"uplink_kbps", uplink_kbps},
    {"cpu_pct", systemMetrics.cpu_pct},
    {"gpu_pct", systemMetrics.gpu_pct},
    {"mem_mb", systemMetrics.mem_mb},
    {"obs", {
      {"dropped_ratio", obsStats.dropped_frames},
      {"encoding_lag_ms", obsStats.encoding_lag_ms},
      {"render_lag_ms", obsStats.render_lag_ms},
      {"streaming", obsStats.streaming},
      {"recording", obsStats.recording},
      {"current_scene", obsStats.current_scene}
    }}
  };
  
  // 메트릭 전송
  IpcLoop::send(m);
  
  // 알림 시스템 업데이트
  alertManager.updateMetrics(m);
}

json Sentinel::runPreflight(){
  return {{"event","preflight_result"},
    {"items", {
      json{{"name","OBS"}, {"status","skip"}, {"detail","ENABLE_OBS=OFF"}},
      json{{"name","Disk"},{"status","ok"},{"detail", ">=10GB free (stub)"}}
    }}};
}

bool Sentinel::startStream(){ 
  return false; 
}

bool Sentinel::stopStream(){ 
  return false; 
}

bool Sentinel::setScene(const std::string&){ 
  return false; 
}
