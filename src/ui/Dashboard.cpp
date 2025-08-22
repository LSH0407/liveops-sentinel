#include "Dashboard.h"
#include "../net/Probe.h"
#include "../obs/ObsClient.h"
#include "../obs/EventLog.h"
#include "../sys/ProcessMon.h"
#include "../alert/Notifier.h"
#include "../core/Metrics.h"
#include "../core/ReportWriter.h"
#include "../diag/BandwidthBench.h"
#include "../diag/Recommendation.h"
#include "../ui/Checklist.h"
#include "../ui/WebhookWizard.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

Dashboard::Dashboard(){
  probe_ = std::make_unique<Probe>();
  obsClient_ = std::make_unique<ObsClient>();
  processMonitor_ = std::make_unique<ProcessMonitor>();
  notifier_ = std::make_unique<Notifier>();
  rttMetrics_ = std::make_unique<MetricsCollector>(600);
  lossMetrics_ = std::make_unique<MetricsCollector>(600);
  bandwidthBench_ = std::make_unique<BandwidthBench>();
  eventLog_ = std::make_unique<obs::EventLog>();
  reportWriter_ = std::make_unique<core::ReportWriter>(core::ReportConfig{});
  checklist_ = std::make_unique<ui::Checklist>(obsClient_, processMonitor_);
  webhookWizard_ = std::make_unique<ui::WebhookWizard>();
  
  // WebhookWizard 콜백 설정
  webhookWizard_->setOnWebhookSaved([this](const std::string& webhook) {
    onWebhookSaved(webhook);
  });
  
  // 기본 모니터링 프로세스 추가
  processMonitor_->addProcess("obs64.exe");
  processMonitor_->addProcess("UnrealEditor.exe");
  
  // OBS 클라이언트 설정
  obsClient_->setStatusCallback([this](const auto& status) {
    // OBS 상태 변경 시 알림
    if (enableAlerts_) {
      if (status.recording) {
        notifier_->sendAlert(AlertLevel::INFO, "OBS Recording Started", 
                           "Recording: " + status.currentProgramScene);
      }
    }
  });
  
  // OBS 이벤트 콜백 설정
  obsClient_->setEventCallback([this](const std::string& eventType, const std::string& payload) {
    eventLog_->push(obs::Event(eventType, nlohmann::json::parse(payload)));
  });
  
  // Probe 시작
  probe_->start(targetHost_, targetPort_, sendRateHz_, [this](double rtt, double loss){
    std::scoped_lock lk(mtx_);
    samples_.push_back(ProbeSample(rtt, loss));
    if (samples_.size() > 600) samples_.erase(samples_.begin());
    
    rttMetrics_->addSample(rtt);
    lossMetrics_->addSample(loss);
    
    checkAlerts();
  });
  
  // OBS 연결 시도
  obsClient_->connect(obsHost_, obsPort_, obsPassword_);
  
  // OBS 이벤트 구독
  obsClient_->subscribeToEvents();
  
  // 첫 실행 시 WebhookWizard 표시
  if (WebhookWizard::shouldShowWizard()) {
    webhookWizard_->show();
  }
}

Dashboard::~Dashboard(){ 
  // 진단 모드 정리
  if (diagnosticMode_) {
    stopDiagnosticMode();
  }
  
  probe_->stop(); 
  obsClient_->disconnect();
}

void Dashboard::draw(){
  // Console application - GUI disabled
  std::cout << "LiveOps Sentinel Console Application" << std::endl;
  std::cout << "GUI functionality disabled in console mode" << std::endl;
  std::cout << "Press Ctrl+C to exit" << std::endl;
}

void Dashboard::drawStatusLights(){
  // Console mode - GUI disabled
  double rtt = samples_.empty()? 0 : samples_.back().rtt_ms;
  double loss = samples_.empty()? 0 : samples_.back().loss_pct;
  bool obsConnected = obsClient_->isConnected();
  
  std::cout << "Status: Probe=" << (rtt>0 && loss<10.0 ? "OK" : "WARN") 
            << " OBS=" << (obsConnected ? "Connected" : "Disconnected") << std::endl;
  std::cout << "RTT: " << rtt << " ms, Loss: " << loss << "%" << std::endl;
}

void Dashboard::drawProbePanel(){
  // Console mode - GUI disabled
  std::cout << "UDP Echo Probe: " << targetHost_ << ":" << targetPort_ << " @ " << sendRateHz_ << "Hz" << std::endl;
}

void Dashboard::drawObsPanel(){
  // Console mode - GUI disabled
  bool connected = obsClient_->isConnected();
  std::cout << "OBS Studio: " << (connected ? "Connected" : "Disconnected") << std::endl;
}

void Dashboard::drawProcessPanel(){
  // Console mode - GUI disabled
  std::cout << "Process Monitor: Active" << std::endl;
}

void Dashboard::drawAlertPanel(){
  // Console mode - GUI disabled
  std::cout << "Alert Configuration: " << (enableAlerts_ ? "Enabled" : "Disabled") << std::endl;
}

void Dashboard::drawConfigPanel(){
  // Console mode - GUI disabled
  std::cout << "Configuration: Console mode" << std::endl;
}

void Dashboard::checkAlerts(){
  if (!enableAlerts_) return;
  
  // Get recent metrics (10 seconds)
  double avgRtt = probe_->getAverageRtt(10);
  double maxRtt = probe_->getMaxRtt(10);
  double avgLoss = probe_->getAverageLoss(10);
  double maxLoss = probe_->getMaxLoss(10);
  
  // Get OBS metrics
  auto obsStatus = obsClient_->getStatus();
  
  // Threshold values (should come from config)
  const double rttThreshold = 80.0;  // ms
  const double lossThreshold = 2.0;  // %
  const int holdSec = 5;  // seconds
  
  static auto rttViolationStart = std::chrono::steady_clock::now();
  static auto lossViolationStart = std::chrono::steady_clock::now();
  static bool rttAlertSent = false;
  static bool lossAlertSent = false;
  
  auto now = std::chrono::steady_clock::now();
  
  // Check RTT threshold with hold time
  if (avgRtt > rttThreshold) {
    if (!rttAlertSent && (now - rttViolationStart) >= std::chrono::seconds(holdSec)) {
      // Create metrics snapshot
      nlohmann::json metricsSnapshot = {
        {"timestamp_range", {
          {"start", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() - 10},
          {"end", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()}
        }},
        {"rtt", {
          {"avg_ms", avgRtt},
          {"max_ms", maxRtt}
        }},
        {"loss", {
          {"avg_pct", avgLoss},
          {"max_pct", maxLoss}
        }},
        {"obs", {
          {"droppedFramesRatio", obsStatus.droppedFramesRatio},
          {"encodingLagMs", obsStatus.encodingLagMs},
          {"renderLagMs", obsStatus.renderLagMs},
          {"cpu_pct", obsStatus.cpuUsage},
          {"gpu_pct", 0.0} // Would need GPU monitoring
        }}
      };
      
      // 쿨다운 기능을 사용한 알림 전송
      notifier_->sendAlertWithCooldown(AlertType::RTT, AlertLevel::WARNING, "High RTT Detected", 
                                      "RTT exceeded threshold for " + std::to_string(holdSec) + " seconds. "
                                      "Avg: " + std::to_string(avgRtt) + " ms, Max: " + std::to_string(maxRtt) + " ms",
                                      avgRtt);
      rttAlertSent = true;
    }
  } else {
    rttViolationStart = now;
    rttAlertSent = false;
  }
  
  // Check Loss threshold with hold time
  if (avgLoss > lossThreshold) {
    if (!lossAlertSent && (now - lossViolationStart) >= std::chrono::seconds(holdSec)) {
      // Create metrics snapshot
      nlohmann::json metricsSnapshot = {
        {"timestamp_range", {
          {"start", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() - 10},
          {"end", std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()}
        }},
        {"rtt", {
          {"avg_ms", avgRtt},
          {"max_ms", maxRtt}
        }},
        {"loss", {
          {"avg_pct", avgLoss},
          {"max_pct", maxLoss}
        }},
        {"obs", {
          {"droppedFramesRatio", obsStatus.droppedFramesRatio},
          {"encodingLagMs", obsStatus.encodingLagMs},
          {"renderLagMs", obsStatus.renderLagMs},
          {"cpu_pct", obsStatus.cpuUsage},
          {"gpu_pct", 0.0} // Would need GPU monitoring
        }}
      };
      
      // 쿨다운 기능을 사용한 알림 전송
      notifier_->sendAlertWithCooldown(AlertType::LOSS, AlertLevel::ERROR, "High Packet Loss Detected", 
                                      "Packet loss exceeded threshold for " + std::to_string(holdSec) + " seconds. "
                                      "Avg: " + std::to_string(avgLoss) + "%, Max: " + std::to_string(maxLoss) + "%",
                                      avgLoss);
      lossAlertSent = true;
    }
  } else {
    lossViolationStart = now;
    lossAlertSent = false;
  }
  
  // OBS 성능 지표 알림 (쿨다운 적용)
  if (obsStatus.droppedFramesRatio > 0.02) {
    notifier_->sendAlertWithCooldown(AlertType::DROPPED_FRAMES, AlertLevel::WARNING, "High Frame Drop Rate", 
                                    "Frame drop rate: " + std::to_string(obsStatus.droppedFramesRatio * 100.0) + "%",
                                    obsStatus.droppedFramesRatio * 100.0);
  }
  
  if (obsStatus.encodingLagMs > 25.0) {
    notifier_->sendAlertWithCooldown(AlertType::ENCODE_LAG, AlertLevel::WARNING, "High Encoding Lag", 
                                    "Encoding lag: " + std::to_string(obsStatus.encodingLagMs) + " ms",
                                    obsStatus.encodingLagMs);
  }
  
  if (obsStatus.renderLagMs > 20.0) {
    notifier_->sendAlertWithCooldown(AlertType::RENDER_LAG, AlertLevel::WARNING, "High Render Lag", 
                                    "Render lag: " + std::to_string(obsStatus.renderLagMs) + " ms",
                                    obsStatus.renderLagMs);
  }
  
  // 진단 모드 자동 시작 조건
  if (!diagnosticMode_ && avgRtt > 80.0 && avgLoss > 1.0) {
    static auto lastAlertTime = std::chrono::steady_clock::now();
    if (now - lastAlertTime > std::chrono::seconds(5)) {
      startDiagnosticMode();
      lastAlertTime = now;
    }
  }
}

void Dashboard::drawBenchmarkPanel() {
  // Console mode - GUI disabled
  std::cout << "Bandwidth Benchmark: Console mode" << std::endl;
}

void Dashboard::drawRecommendationPanel() {
  // Console mode - GUI disabled
  std::cout << "OBS Settings Recommendation: Console mode" << std::endl;
}

void Dashboard::calculateRecommendation() {
  // Console mode - GUI disabled
  std::cout << "Calculate Recommendation: Console mode" << std::endl;
}

void Dashboard::startDiagnosticMode() {
  if (diagnosticMode_) return; // 이미 실행 중이면 무시
  
  diagnosticMode_ = true;
  diagnosticStart_ = std::chrono::steady_clock::now();
  diagnosticData_.clear();
  
  // 원본 프로브 레이트 저장
  originalProbeRate_ = sendRateHz_;
  
  // 프로브 레이트를 60Hz로 증가 (고주기 샘플링)
  probe_->stop();
  probe_->start(targetHost_, targetPort_, 60, [this](double rtt, double loss){
    std::scoped_lock lk(mtx_);
    samples_.push_back(ProbeSample(rtt, loss));
    if (samples_.size() > 600) samples_.erase(samples_.begin());
    
    rttMetrics_->addSample(rtt);
    lossMetrics_->addSample(loss);
    
    // 진단 데이터 수집
    if (diagnosticMode_) {
      auto obsStatus = obsClient_->getStatus();
      DiagnosticData data;
      data.timestamp = std::chrono::steady_clock::now();
      data.rtt_ms = rtt;
      data.loss_pct = loss;
      data.droppedFramesRatio = obsStatus.droppedFramesRatio;
      data.encodingLagMs = obsStatus.encodingLagMs;
      data.renderLagMs = obsStatus.renderLagMs;
      data.cpu_pct = obsStatus.cpuUsage;
      data.gpu_pct = 0.0; // GPU 모니터링 추가 필요
      data.mem_mb = obsStatus.memoryUsageMB;
      data.diskWriteMBps = 0.0; // 디스크 쓰기 속도 측정 추가 필요
      
      diagnosticData_.push_back(data);
    }
    
    checkAlerts();
  });
  
  // 진단 스레드 시작
  diagnosticRunning_ = true;
  diagnosticThread_ = std::thread([this]() {
    while (diagnosticRunning_) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      
      auto now = std::chrono::steady_clock::now();
      if (now - diagnosticStart_ >= diagnosticDuration_) {
        stopDiagnosticMode();
        break;
      }
    }
  });
  
  notifier_->sendAlert(AlertLevel::WARNING, "Diagnostic Mode Started", 
                      "Automatic diagnostic mode activated due to network issues. Duration: 5 minutes.");
  
  spdlog::info("Diagnostic mode started - high frequency sampling enabled");
}

void Dashboard::stopDiagnosticMode() {
  if (!diagnosticMode_) return;
  
  diagnosticMode_ = false;
  diagnosticRunning_ = false;
  
  if (diagnosticThread_.joinable()) {
    diagnosticThread_.join();
  }
  
  // 프로브 레이트를 원래대로 복원
  probe_->stop();
  probe_->start(targetHost_, targetPort_, originalProbeRate_, [this](double rtt, double loss){
    std::scoped_lock lk(mtx_);
    samples_.push_back(ProbeSample(rtt, loss));
    if (samples_.size() > 600) samples_.erase(samples_.begin());
    
    rttMetrics_->addSample(rtt);
    lossMetrics_->addSample(loss);
    
    checkAlerts();
  });
  
  // 진단 리포트 저장
  saveDiagnosticReport();
  
  notifier_->sendAlert(AlertLevel::INFO, "Diagnostic Mode Completed", 
                      "Diagnostic mode completed. Report saved.");
  
  spdlog::info("Diagnostic mode stopped - report saved");
}

void Dashboard::saveDiagnosticReport() {
  if (diagnosticData_.empty()) return;
  
  std::filesystem::create_directories("reports");
  
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << "reports/diag_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M") << ".json";
  
  nlohmann::json report;
  report["metadata"] = {
    {"startTime", std::chrono::duration_cast<std::chrono::seconds>(diagnosticStart_.time_since_epoch()).count()},
    {"endTime", std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count()},
    {"durationSec", std::chrono::duration_cast<std::chrono::seconds>(diagnosticDuration_).count()},
    {"totalSamples", diagnosticData_.size()},
    {"samplingRateHz", 60}
  };
  
  report["summary"] = {
    {"avgRtt", 0.0},
    {"maxRtt", 0.0},
    {"avgLoss", 0.0},
    {"maxLoss", 0.0},
    {"avgDroppedFrames", 0.0},
    {"maxDroppedFrames", 0.0},
    {"avgCpu", 0.0},
    {"maxCpu", 0.0}
  };
  
  // 통계 계산
  double sumRtt = 0.0, maxRtt = 0.0;
  double sumLoss = 0.0, maxLoss = 0.0;
  double sumDropped = 0.0, maxDropped = 0.0;
  double sumCpu = 0.0, maxCpu = 0.0;
  
  for (const auto& data : diagnosticData_) {
    sumRtt += data.rtt_ms;
    maxRtt = std::max(maxRtt, data.rtt_ms);
    sumLoss += data.loss_pct;
    maxLoss = std::max(maxLoss, data.loss_pct);
    sumDropped += data.droppedFramesRatio;
    maxDropped = std::max(maxDropped, data.droppedFramesRatio);
    sumCpu += data.cpu_pct;
    maxCpu = std::max(maxCpu, data.cpu_pct);
  }
  
  int count = diagnosticData_.size();
  report["summary"]["avgRtt"] = sumRtt / count;
  report["summary"]["maxRtt"] = maxRtt;
  report["summary"]["avgLoss"] = sumLoss / count;
  report["summary"]["maxLoss"] = maxLoss;
  report["summary"]["avgDroppedFrames"] = sumDropped / count;
  report["summary"]["maxDroppedFrames"] = maxDropped;
  report["summary"]["avgCpu"] = sumCpu / count;
  report["summary"]["maxCpu"] = maxCpu;
  
  // 상세 데이터
  nlohmann::json samples = nlohmann::json::array();
  for (const auto& data : diagnosticData_) {
    samples.push_back({
      {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(data.timestamp.time_since_epoch()).count()},
      {"rtt_ms", data.rtt_ms},
      {"loss_pct", data.loss_pct},
      {"droppedFramesRatio", data.droppedFramesRatio},
      {"encodingLagMs", data.encodingLagMs},
      {"renderLagMs", data.renderLagMs},
      {"cpu_pct", data.cpu_pct},
      {"gpu_pct", data.gpu_pct},
      {"mem_mb", data.mem_mb},
      {"diskWriteMBps", data.diskWriteMBps}
    });
  }
  report["samples"] = samples;
  
  // JSON 파일 저장
  std::ofstream jsonFile(ss.str());
  jsonFile << report.dump(2);
  
  // CSV 파일도 저장
  std::string csvPath = ss.str().substr(0, ss.str().length() - 5) + ".csv";
  std::ofstream csvFile(csvPath);
  csvFile << "timestamp,rtt_ms,loss_pct,droppedFramesRatio,encodingLagMs,renderLagMs,cpu_pct,gpu_pct,mem_mb,diskWriteMBps\n";
  
  for (const auto& data : diagnosticData_) {
    csvFile << std::chrono::duration_cast<std::chrono::milliseconds>(data.timestamp.time_since_epoch()).count() << ","
            << data.rtt_ms << ","
            << data.loss_pct << ","
            << data.droppedFramesRatio << ","
            << data.encodingLagMs << ","
            << data.renderLagMs << ","
            << data.cpu_pct << ","
            << data.gpu_pct << ","
            << data.mem_mb << ","
            << data.diskWriteMBps << "\n";
  }
  
  spdlog::info("Diagnostic report saved: {} and {}", ss.str(), csvPath);
}

void Dashboard::saveReport() {
  // Console mode - GUI disabled
  std::cout << "Save Report: Console mode" << std::endl;
}

void Dashboard::drawControlPanel() {
  // Console mode - GUI disabled
  std::cout << "OBS Control: Console mode" << std::endl;
}

void Dashboard::drawChecklistPanel() {
  // Console mode - GUI disabled
  std::cout << "Checklist: Console mode" << std::endl;
}

void Dashboard::drawStatsDetailPanel() {
  // Console mode - GUI disabled
  std::cout << "OBS Stats Detail: Console mode" << std::endl;
}

void Dashboard::drawReportsPanel() {
  // Console mode - GUI disabled
  std::cout << "Reports: Console mode" << std::endl;
}

void Dashboard::drawSettingsPanel() {
  // Console mode - GUI disabled
  std::cout << "Settings: Console mode" << std::endl;
}

void Dashboard::drawWebhookBanner() {
  // Console mode - GUI disabled
  if (!notifier_->isWebhookConfigured()) {
    std::cout << "Warning: Discord Webhook not configured" << std::endl;
  }
}

void Dashboard::onWebhookSaved(const std::string& webhook) {
  // Notifier 설정 업데이트
  AlertConfig alertConfig;
  alertConfig.discordWebhook = webhook;
  alertConfig.webhookConfigured = true;
  alertConfig.enableDiscord = true;
  alertConfig.discordUsername = "LiveOps Sentinel";
  alertConfig.cooldownSec = 60;
  notifier_->setConfig(alertConfig);
  
  spdlog::info("Webhook saved and notifier updated: {}", MaskWebhook(webhook));
}
