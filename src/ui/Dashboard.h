#pragma once
#include <vector>
#include <mutex>
#include <chrono>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

// ProbeSample is defined in Probe.h

struct DiagnosticData {
  std::chrono::steady_clock::time_point timestamp;
  double rtt_ms;
  double loss_pct;
  double droppedFramesRatio;
  double encodingLagMs;
  double renderLagMs;
  double cpu_pct;
  double gpu_pct;
  double mem_mb;
  double diskWriteMBps;
};

class Probe; // fwd
class ObsClient; // fwd
class ProcessMonitor; // fwd
class Notifier; // fwd
class MetricsCollector; // fwd
class BandwidthBench; // fwd
class EventLog; // fwd
class ReportWriter; // fwd
class Checklist; // fwd
class WebhookWizard; // fwd

class Dashboard {
public:
  Dashboard();
  ~Dashboard();
  void draw();
private:
  void drawProbePanel();
  void drawStatusLights();
  void drawObsPanel();
  void drawProcessPanel();
  void drawAlertPanel();
  void drawConfigPanel();
  void drawBenchmarkPanel();
  void drawRecommendationPanel();
  void drawControlPanel();
  void drawChecklistPanel();
  void drawReportsPanel();
  void drawStatsDetailPanel();
  void drawSettingsPanel();
  void drawWebhookBanner();
  void checkAlerts();
  void startDiagnosticMode();
  void stopDiagnosticMode();
  void saveDiagnosticReport();
  void saveReport();
  void onWebhookSaved(const std::string& webhook);

  std::unique_ptr<Probe> probe_;
  std::unique_ptr<ObsClient> obsClient_;
  std::unique_ptr<ProcessMonitor> processMonitor_;
  std::unique_ptr<Notifier> notifier_;
  std::unique_ptr<MetricsCollector> rttMetrics_;
  std::unique_ptr<MetricsCollector> lossMetrics_;
  std::unique_ptr<BandwidthBench> bandwidthBench_;
  std::unique_ptr<EventLog> eventLog_;
  std::unique_ptr<ReportWriter> reportWriter_;
  std::unique_ptr<Checklist> checklist_;
  std::unique_ptr<WebhookWizard> webhookWizard_;
  
  std::vector<ProbeSample> samples_;
  std::mutex mtx_;
  
  // 설정
  std::string targetHost_{"127.0.0.1"};
  int targetPort_{50051};
  int sendRateHz_{20};
  std::string obsHost_{"localhost"};
  int obsPort_{4455};
  std::string obsPassword_;
  
  // 알림 설정
  double rttThreshold_{100.0};
  double lossThreshold_{5.0};
  bool enableAlerts_{true};
  
  // 벤치마크 상태
  bool benchmarkRunning_{false};
  // BenchResult lastBenchResult_; // Console mode - disabled
  // std::vector<BenchResult> benchHistory_; // Console mode - disabled
  
  // 추천 상태
  // ObsRecommendation currentRecommendation_; // Console mode - disabled
  bool showRecommendation_{false};
  
  // 진단 모드
  bool diagnosticMode_{false};
  std::chrono::steady_clock::time_point diagnosticStart_;
  std::chrono::seconds diagnosticDuration_{300}; // 5분
  std::vector<DiagnosticData> diagnosticData_;
  std::thread diagnosticThread_;
  std::atomic<bool> diagnosticRunning_{false};
  int originalProbeRate_{20};
  
  // UI 상태
  bool showConfig_{false};
  bool showProcesses_{true};
  bool showAlerts_{true};
  bool showBenchmark_{false};
  bool showControl_{false};
  bool showChecklist_{false};
  bool showReports_{false};
  bool showStatsDetail_{false};
  bool showSettings_{false};
};
