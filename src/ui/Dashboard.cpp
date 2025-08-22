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
    samples_.push_back({rtt, loss, std::chrono::steady_clock::now()});
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
  ImGui::Begin("LiveOps Sentinel");
  
  // 메뉴바
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Configuration", nullptr, &showConfig_);
      ImGui::MenuItem("Process Monitor", nullptr, &showProcesses_);
      ImGui::MenuItem("Alerts", nullptr, &showAlerts_);
      ImGui::MenuItem("Benchmark", nullptr, &showBenchmark_);
      ImGui::MenuItem("Control", nullptr, &showControl_);
      ImGui::MenuItem("Checklist", nullptr, &showChecklist_);
      ImGui::MenuItem("Reports", nullptr, &showReports_);
      ImGui::MenuItem("Stats Detail", nullptr, &showStatsDetail_);
      ImGui::MenuItem("Settings", nullptr, &showSettings_);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
  
  // Webhook 경고 배너 표시
  drawWebhookBanner();
  
  drawStatusLights();
  ImGui::Separator();
  drawProbePanel();
  ImGui::Separator();
  drawObsPanel();
  
  if (showProcesses_) {
    ImGui::Separator();
    drawProcessPanel();
  }
  
  if (showAlerts_) {
    ImGui::Separator();
    drawAlertPanel();
  }
  
  if (showBenchmark_) {
    ImGui::Separator();
    drawBenchmarkPanel();
  }
  
  if (showRecommendation_) {
    ImGui::Separator();
    drawRecommendationPanel();
  }
  
  if (showControl_) {
    ImGui::Separator();
    drawControlPanel();
  }
  
  if (showChecklist_) {
    ImGui::Separator();
    drawChecklistPanel();
  }
  
  if (showReports_) {
    ImGui::Separator();
    drawReportsPanel();
  }
  
  if (showStatsDetail_) {
    ImGui::Separator();
    drawStatsDetailPanel();
  }
  
  if (showConfig_) {
    ImGui::Separator();
    drawConfigPanel();
  }
  
  if (showSettings_) {
    ImGui::Separator();
    drawSettingsPanel();
  }
  
  // WebhookWizard 모달 표시
  webhookWizard_->show();
  
  ImGui::End();
}

void Dashboard::drawStatusLights(){
  double rtt = samples_.empty()? 0 : samples_.back().rtt_ms;
  double loss = samples_.empty()? 0 : samples_.back().loss_pct;
  bool obsConnected = obsClient_->isConnected();
  
  auto light = [&](const char* name, bool ok){
    ImGui::Text("%s", name);
    ImGui::SameLine(); ImGui::TextColored(ok?ImVec4(0,1,0,1):ImVec4(1,0.7f,0,1), "●");
  };
  
  light("Probe", rtt>0 && loss<10.0);
  light("OBS", obsConnected);
  
  // 진단 모드 상태 표시
  if (diagnosticMode_) {
    auto elapsed = std::chrono::steady_clock::now() - diagnosticStart_;
    auto remaining = diagnosticDuration_ - elapsed;
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Diagnostic Mode: %llds remaining", 
                       std::chrono::duration_cast<std::chrono::seconds>(remaining).count());
  }
  
  ImGui::Text("RTT: %.2f ms   Loss: %.1f %%", rtt, loss);
  
  // 통계 정보
  if (rttMetrics_->getSampleCount() > 0) {
    ImGui::Text("Avg RTT: %.2f ms   Min: %.2f ms   Max: %.2f ms", 
                rttMetrics_->getAverage(), rttMetrics_->getMin(), rttMetrics_->getMax());
  }
}

void Dashboard::drawProbePanel(){
  ImGui::Text("UDP Echo Probe");
  
  // 설정
  if (ImGui::InputText("Host", &targetHost_)) {
    // 호스트 변경 시 자동 재시작
  }
  if (ImGui::InputInt("Port", &targetPort_)) {
    // 포트 변경 시 자동 재시작
  }
  ImGui::SliderInt("Send Rate (Hz)", &sendRateHz_, 1, 60);
  
  if (ImGui::Button("Restart Probe")){
    probe_->stop();
    probe_->start(targetHost_, targetPort_, sendRateHz_, [this](double rtt, double loss){
      std::scoped_lock lk(mtx_);
      samples_.push_back({rtt, loss, std::chrono::steady_clock::now()});
      if (samples_.size() > 600) samples_.erase(samples_.begin());
      rttMetrics_->addSample(rtt);
      lossMetrics_->addSample(loss);
      checkAlerts();
    });
  }
  
  // 그래프
  ImGui::Text("Network Performance");
  
  // RTT 그래프
  static float rttBuf[600];
  size_t n = samples_.size();
  for (size_t i=0;i<600;i++) rttBuf[i] = (i<n)? (float)samples_[i].rtt_ms : 0.f;
  ImGui::PlotLines("RTT (ms)", rttBuf, (int)600, 0, nullptr, 0.f, 200.f, ImVec2(-1,80));
  
  // Loss 그래프
  static float lossBuf[600];
  for (size_t i=0;i<600;i++) lossBuf[i] = (i<n)? (float)samples_[i].loss_pct : 0.f;
  ImGui::PlotLines("Packet Loss (%)", lossBuf, (int)600, 0, nullptr, 0.f, 20.f, ImVec2(-1,80));
}

void Dashboard::drawObsPanel(){
  ImGui::Text("OBS Studio Status");
  
  bool connected = obsClient_->isConnected();
  ImGui::Text("Connection: %s", connected ? "Connected" : "Disconnected");
  
  if (connected) {
    auto status = obsClient_->getStatus();
    
    ImGui::Text("Recording: %s", status.recording ? "ON" : "OFF");
    ImGui::Text("Streaming: %s", status.streaming ? "ON" : "OFF");
    ImGui::Text("Current Scene: %s", status.currentProgramScene.c_str());
    
    // OBS 연결 설정
    ImGui::Separator();
    ImGui::Text("Connection Settings");
    if (ImGui::InputText("OBS Host", &obsHost_)) {
      // 호스트 변경 시 재연결
    }
    if (ImGui::InputInt("OBS Port", &obsPort_)) {
      // 포트 변경 시 재연결
    }
    ImGui::InputText("Password", &obsPassword_);
    
    if (ImGui::Button("Reconnect to OBS")) {
      obsClient_->disconnect();
      obsClient_->connect(obsHost_, obsPort_, obsPassword_);
    }
  } else {
    ImGui::TextColored(ImVec4(1,0.7f,0,1), "Failed to connect to OBS WebSocket");
    ImGui::Text("Make sure OBS Studio is running with obs-websocket plugin enabled");
    
    if (ImGui::Button("Retry Connection")) {
      obsClient_->connect(obsHost_, obsPort_, obsPassword_);
    }
  }
}

void Dashboard::drawProcessPanel(){
  ImGui::Text("Process Monitor");
  
  auto stats = processMonitor_->getProcessStats();
  
  for (const auto& stat : stats) {
    ImGui::Text("%s (PID: %d)", stat.name.c_str(), stat.pid);
    ImGui::SameLine();
    ImGui::TextColored(stat.running ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), 
                       stat.running ? "●" : "○");
    
    if (stat.running) {
      ImGui::Text("  Memory: %.1f MB", stat.mem_mb);
    }
  }
  
  // 새 프로세스 추가
  static char newProcess[256] = "";
  ImGui::InputText("Add Process", newProcess, sizeof(newProcess));
  ImGui::SameLine();
  if (ImGui::Button("Add")) {
    if (strlen(newProcess) > 0) {
      processMonitor_->addProcess(newProcess);
      newProcess[0] = '\0';
    }
  }
}

void Dashboard::drawAlertPanel(){
  ImGui::Text("Alert Configuration");
  
  ImGui::Checkbox("Enable Alerts", &enableAlerts_);
  ImGui::SliderFloat("RTT Threshold (ms)", &rttThreshold_, 10.0f, 500.0f, "%.1f");
  ImGui::SliderFloat("Loss Threshold (%)", &lossThreshold_, 0.1f, 50.0f, "%.1f");
  
  if (ImGui::Button("Test Alert")) {
    notifier_->sendAlert(AlertLevel::INFO, "Test Alert", "This is a test alert from LiveOps Sentinel");
  }
  
  ImGui::Separator();
  ImGui::Text("Recent Alerts");
  ImGui::TextDisabled("Alert history will be displayed here");
}

void Dashboard::drawConfigPanel(){
  ImGui::Text("Configuration");
  
  if (ImGui::Button("Save Config")) {
    // 설정 저장 로직
  }
  
  if (ImGui::Button("Load Config")) {
    // 설정 로드 로직
  }
  
  ImGui::Separator();
  ImGui::Text("Discord Webhook");
  static char webhook[512] = "";
  ImGui::InputText("Webhook URL", webhook, sizeof(webhook));
  
  if (ImGui::Button("Test Discord")) {
    notifier_->sendDiscordAlert("Test message from LiveOps Sentinel");
  }
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
  ImGui::Text("Bandwidth Benchmark");
  
  static BenchConfig benchConfig;
  benchConfig.durationSec = 30;
  benchConfig.protocol = BenchProtocol::UDP;
  benchConfig.mode = BenchMode::CLIENT;
  benchConfig.targetHost = "127.0.0.1";
  benchConfig.targetPort = 50052;
  
  // 설정
  ImGui::Text("Configuration");
  ImGui::InputText("Target Host", &benchConfig.targetHost);
  ImGui::InputInt("Target Port", &benchConfig.targetPort);
  ImGui::InputInt("Duration (sec)", &benchConfig.durationSec);
  
  static int protocolIndex = 0;
  const char* protocols[] = {"UDP", "TCP"};
  ImGui::Combo("Protocol", &protocolIndex, protocols, 2);
  benchConfig.protocol = static_cast<BenchProtocol>(protocolIndex);
  
  static int modeIndex = 0;
  const char* modes[] = {"Client", "Server"};
  ImGui::Combo("Mode", &modeIndex, modes, 2);
  benchConfig.mode = static_cast<BenchMode>(modeIndex);
  
  // 제어 버튼
  if (!benchmarkRunning_) {
    if (ImGui::Button("Start Benchmark")) {
      benchmarkRunning_ = true;
      bandwidthBench_->start(benchConfig, [this](const BenchResult& result) {
        lastBenchResult_ = result;
        benchHistory_.push_back(result);
        if (benchHistory_.size() > 10) {
          benchHistory_.erase(benchHistory_.begin());
        }
        benchmarkRunning_ = false;
        
        // 추천 계산
        calculateRecommendation();
      });
    }
  } else {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Benchmark running...");
    if (ImGui::Button("Stop")) {
      bandwidthBench_->stop();
      benchmarkRunning_ = false;
    }
  }
  
  // 결과 표시
  if (!benchHistory_.empty()) {
    ImGui::Separator();
    ImGui::Text("Latest Results");
    
    const auto& result = benchHistory_.back();
    ImGui::Text("Uplink: %.2f Mbps", result.uplinkMbps);
    ImGui::Text("Loss: %.2f%%", result.lossPct);
    ImGui::Text("RTT Avg: %.2f ms", result.rttMsAvg);
    ImGui::Text("RTT Min/Max: %.2f/%.2f ms", result.rttMsMin, result.rttMsMax);
    ImGui::Text("Jitter: %.2f ms", result.jitterMs);
    ImGui::Text("Packets: %d/%d", result.receivedPackets, result.totalPackets);
  }
}

void Dashboard::drawRecommendationPanel() {
  ImGui::Text("OBS Settings Recommendation");
  
  if (showRecommendation_) {
    ImGui::Text("Encoder: %s", 
                currentRecommendation_.encoder == EncoderType::NVENC ? "NVENC" : 
                currentRecommendation_.encoder == EncoderType::X264 ? "x264" : "QSV");
    ImGui::Text("Bitrate: %d kbps", currentRecommendation_.bitrateKbps);
    ImGui::Text("Keyframe: %d sec", currentRecommendation_.keyframeSec);
    ImGui::Text("VBV Buffer: %d kbps", currentRecommendation_.vbvBufferKbps);
    
    const char* presetNames[] = {"Quality", "Performance", "Ultrafast", "Veryfast", "Fast", "Medium", "Slow", "Slower", "Veryslow"};
    ImGui::Text("Preset: %s", presetNames[static_cast<int>(currentRecommendation_.preset)]);
    ImGui::Text("Profile: %s", currentRecommendation_.profile.c_str());
    
    ImGui::Text("Notes: %s", currentRecommendation_.notes.c_str());
    
    if (ImGui::Button("Apply as Preset (JSON)")) {
      saveReport();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Open Reports Folder")) {
      std::filesystem::create_directories("reports");
      system("explorer reports"); // Windows
    }
  } else {
    ImGui::TextDisabled("Run benchmark to get recommendations");
  }
}

void Dashboard::calculateRecommendation() {
  if (benchHistory_.empty()) return;
  
  const auto& benchResult = benchHistory_.back();
  auto obsStatus = obsClient_->getStatus();
  
  RecommendationInput input;
  input.network.sustainedUplinkMbps = benchResult.uplinkMbps;
  input.network.rttMs = benchResult.rttMsAvg;
  input.network.lossPct = benchResult.lossPct;
  input.network.jitterMs = benchResult.jitterMs;
  
  input.obs.droppedFramesRatio = obsStatus.droppedFramesRatio;
  input.obs.averageFrameRenderTimeMs = obsStatus.averageFrameRenderTimeMs;
  input.obs.cpuUsage = obsStatus.cpuUsage;
  input.obs.memoryUsageMB = obsStatus.memoryUsageMB;
  input.obs.encodingLagMs = obsStatus.encodingLagMs;
  input.obs.renderLagMs = obsStatus.renderLagMs;
  
  // 비디오 설정 (OBS에서 가져온 값 사용)
  input.video.baseWidth = obsStatus.baseWidth;
  input.video.baseHeight = obsStatus.baseHeight;
  input.video.outputWidth = obsStatus.outputWidth;
  input.video.outputHeight = obsStatus.outputHeight;
  input.video.fps = obsStatus.fps;
  
  // 시스템 메트릭 (간단한 구현)
  auto processStats = processMonitor_->getProcessStats();
  for (const auto& stat : processStats) {
    if (stat.name.find("obs") != std::string::npos) {
      input.system.cpuPct = stat.cpu_pct;
      input.system.memoryMB = stat.mem_mb;
      break;
    }
  }
  
  // 벤치마크 결과에서 시스템 지표 가져오기
  if (!benchResult.systemMetrics.timestamp.time_since_epoch().count()) {
    input.system.cpuPct = benchResult.systemMetrics.cpuPct;
    input.system.gpuPct = benchResult.systemMetrics.gpuPct;
    input.system.diskWriteMBps = benchResult.systemMetrics.diskWriteMBps;
  }
  
  input.preferredEncoder = EncoderType::NVENC; // 기본값
  input.headroom = 0.75; // 기본값
  input.minKbps = 800;
  input.maxKbps = 15000;
  
  currentRecommendation_ = RecommendationEngine::RecommendObsSettings(input);
  showRecommendation_ = true;
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
    samples_.push_back({rtt, loss, std::chrono::steady_clock::now()});
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
    samples_.push_back({rtt, loss, std::chrono::steady_clock::now()});
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
  std::filesystem::create_directories("reports");
  
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << "reports/obs_preset_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M") << ".json";
  
  nlohmann::json report;
  report["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  report["recommendation"] = {
    {"encoder", currentRecommendation_.encoder == EncoderType::NVENC ? "nvenc" : "x264"},
    {"bitrateKbps", currentRecommendation_.bitrateKbps},
    {"keyframeSec", currentRecommendation_.keyframeSec},
    {"vbvBufferKbps", currentRecommendation_.vbvBufferKbps},
    {"preset", "quality"}, // 간단화
    {"profile", currentRecommendation_.profile},
    {"notes", currentRecommendation_.notes}
  };
  
  if (!benchHistory_.empty()) {
    const auto& bench = benchHistory_.back();
    report["benchmark"] = {
      {"uplinkMbps", bench.uplinkMbps},
      {"lossPct", bench.lossPct},
      {"rttMsAvg", bench.rttMsAvg}
    };
  }
  
  std::ofstream file(ss.str());
  file << report.dump(2);
  
  notifier_->sendAlert(AlertLevel::INFO, "Report Saved", 
                      "OBS preset saved to: " + ss.str());
}

void Dashboard::drawControlPanel() {
  ImGui::Begin("OBS Control");
  
  if (!obsClient_->isConnected()) {
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "OBS not connected");
    ImGui::End();
    return;
  }
  
  auto status = obsClient_->getStatus();
  
  // Scene Control
  ImGui::Text("Scene Control");
  static std::vector<std::string> scenes;
  static int selectedScene = 0;
  static bool scenesLoaded = false;
  
  if (!scenesLoaded) {
    obsClient_->getSceneList(scenes);
    scenesLoaded = true;
  }
  
  if (ImGui::BeginCombo("Current Scene", status.currentProgramScene.c_str())) {
    for (int i = 0; i < scenes.size(); i++) {
      const bool isSelected = (selectedScene == i);
      if (ImGui::Selectable(scenes[i].c_str(), isSelected)) {
        selectedScene = i;
        obsClient_->setCurrentProgramScene(scenes[i]);
      }
      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  
  ImGui::Separator();
  
  // Stream Control
  ImGui::Text("Stream Control");
  if (status.streaming) {
    if (ImGui::Button("Stop Stream", ImVec2(120, 30))) {
      obsClient_->stopStream();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Streaming");
  } else {
    if (ImGui::Button("Start Stream", ImVec2(120, 30))) {
      obsClient_->startStream();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Streaming");
  }
  
  ImGui::Separator();
  
  // Record Control
  ImGui::Text("Record Control");
  if (status.recording) {
    if (ImGui::Button("Stop Record", ImVec2(120, 30))) {
      obsClient_->stopRecord();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Recording");
  } else {
    if (ImGui::Button("Start Record", ImVec2(120, 30))) {
      obsClient_->startRecord();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not Recording");
  }
  
  ImGui::Separator();
  
  // Event Log
  ImGui::Text("Event Log");
  static char searchFilter[256] = "";
  ImGui::InputText("Search Events", searchFilter, sizeof(searchFilter));
  
  auto events = eventLog_->getRecentEvents(200);
  if (!searchFilter[0]) {
    // Show all events
    for (const auto& event : events) {
      auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
      ImGui::Text("%s - %s", 
                  std::put_time(std::localtime(&time_t), "%H:%M:%S").c_str(),
                  event.type.c_str());
    }
  } else {
    // Show filtered events
    auto filteredEvents = eventLog_->searchEvents(searchFilter, 100);
    for (const auto& event : filteredEvents) {
      auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
      ImGui::Text("%s - %s", 
                  std::put_time(std::localtime(&time_t), "%H:%M:%S").c_str(),
                  event.type.c_str());
    }
  }
  
  if (ImGui::Button("Save Event Log")) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "reports/events_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M") << ".json";
    eventLog_->saveToJson(ss.str());
  }
  
  ImGui::End();
}

void Dashboard::drawChecklistPanel() {
  checklist_->draw();
}

void Dashboard::drawStatsDetailPanel() {
  ImGui::Begin("OBS Stats Detail");
  
  auto status = obsClient_->getStatus();
  
  if (!status.connected) {
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "OBS not connected");
    ImGui::End();
    return;
  }
  
  ImGui::Text("OBS Performance Statistics");
  ImGui::Separator();
  
  // 비디오 설정
  ImGui::Text("Video Settings");
  ImGui::Text("Base Resolution: %dx%d", status.baseWidth, status.baseHeight);
  ImGui::Text("Output Resolution: %dx%d", status.outputWidth, status.outputHeight);
  ImGui::Text("Frame Rate: %.2f FPS", status.fps);
  
  ImGui::Separator();
  
  // 성능 지표
  ImGui::Text("Performance Metrics");
  
  // 프레임 드롭 관련
  ImGui::Text("Dropped Frames Ratio: %.3f%%", status.droppedFramesRatio * 100.0);
  ImGui::Text("Output Skipped Frames: %.3f%%", status.outputSkippedFrames * 100.0);
  
  // 렌더링 지연
  ImGui::Text("Average Frame Render Time: %.2f ms", status.averageFrameRenderTimeMs);
  ImGui::Text("Render Lag: %.2f ms", status.renderLagMs);
  ImGui::Text("Encoding Lag: %.2f ms", status.encodingLagMs);
  
  // 시스템 리소스
  ImGui::Text("CPU Usage: %.1f%%", status.cpuUsage);
  ImGui::Text("Memory Usage: %.1f MB", status.memoryUsageMB);
  ImGui::Text("Active FPS: %.2f", status.activeFps);
  
  // 출력 정보
  ImGui::Text("Output Bytes: %lld", status.outputBytes);
  ImGui::Text("Strain: %.2f", status.strain);
  
  ImGui::Separator();
  
  // 상태 표시
  ImGui::Text("Status");
  ImGui::Text("Streaming: %s", status.streaming ? "Yes" : "No");
  ImGui::Text("Recording: %s", status.recording ? "Yes" : "No");
  ImGui::Text("Current Scene: %s", status.currentProgramScene.c_str());
  
  // 새로고침 버튼
  if (ImGui::Button("Refresh Stats", ImVec2(120, 30))) {
    obsClient_->getVideoSettings();
  }
  
  ImGui::End();
}

void Dashboard::drawReportsPanel() {
  ImGui::Begin("Reports");
  
  ImGui::Text("Report Export");
  
  if (ImGui::Button("Flush Now", ImVec2(120, 30))) {
    reportWriter_->flushNow();
  }
  
  ImGui::SameLine();
  if (ImGui::Button("Open Folder", ImVec2(120, 30))) {
    reportWriter_->openReportsFolder();
  }
  
  ImGui::Separator();
  
  ImGui::Text("Recent Report Files");
  auto recentFiles = reportWriter_->getRecentReportFiles();
  
  for (const auto& file : recentFiles) {
    if (ImGui::Selectable(file.c_str())) {
      // Open file in explorer
      std::string fullPath = "reports/" + file;
      std::filesystem::path path(fullPath);
      if (std::filesystem::exists(path)) {
#ifdef _WIN32
        system(("explorer " + fullPath).c_str());
#else
        system(("xdg-open " + fullPath).c_str());
#endif
      }
    }
  }
  
  ImGui::End();
}

void Dashboard::drawSettingsPanel() {
  ImGui::Begin("Settings");
  
  ImGui::Text("Discord Webhook Configuration");
  ImGui::Separator();
  
  // 현재 웹훅 상태 표시
  Config config;
  LoadUserConfig(config);
  
  if (config.webhookConfigured && !config.discordWebhook.empty()) {
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "✅ Webhook Configured");
    ImGui::Text("URL: %s", MaskWebhook(config.discordWebhook).c_str());
    
    ImGui::Spacing();
    
    // 버튼들
    if (ImGui::Button("Change Webhook", ImVec2(120, 30))) {
      webhookWizard_->show();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Copy URL", ImVec2(120, 30))) {
      // 클립보드에 복사 (마스킹된 URL)
      ImGui::SetClipboardText(MaskWebhook(config.discordWebhook).c_str());
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Reset Webhook", ImVec2(120, 30))) {
      config.discordWebhook.clear();
      config.webhookConfigured = false;
      SaveUserConfig(config);
      
      // Notifier 설정 업데이트
      AlertConfig alertConfig;
      alertConfig.discordWebhook = "";
      alertConfig.webhookConfigured = false;
      notifier_->setConfig(alertConfig);
    }
    
    ImGui::Spacing();
    if (ImGui::Button("Test Webhook", ImVec2(120, 30))) {
      notifier_->sendDiscordAlert("LiveOps Sentinel webhook test: ✅ Settings panel test");
    }
  } else {
    ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "⚠️ Webhook Not Configured");
    ImGui::Text("Discord 알림을 받으려면 웹훅을 설정하세요.");
    
    ImGui::Spacing();
    if (ImGui::Button("Configure Webhook", ImVec2(150, 30))) {
      webhookWizard_->show();
    }
  }
  
  ImGui::Separator();
  
  // 기타 설정들
  ImGui::Text("Alert Settings");
  ImGui::SliderFloat("RTT Threshold (ms)", &rttThreshold_, 10.0f, 500.0f, "%.1f");
  ImGui::SliderFloat("Loss Threshold (%)", &lossThreshold_, 0.1f, 20.0f, "%.1f");
  ImGui::Checkbox("Enable Alerts", &enableAlerts_);
  
  ImGui::End();
}

void Dashboard::drawWebhookBanner() {
  if (!notifier_->isWebhookConfigured()) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 0.8f, 0.0f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.8f, 0.0f, 0.8f));
    
    if (ImGui::BeginChild("WebhookBanner", ImVec2(0, 40), true)) {
      ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.0f, 1.0f), "⚠️ Discord Webhook이 설정되지 않았습니다.");
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Settings → Webhook에서 설정하세요.");
      
      ImGui::SameLine();
      if (ImGui::Button("Configure Now", ImVec2(100, 20))) {
        showSettings_ = true;
      }
    }
    ImGui::EndChild();
    
    ImGui::PopStyleColor(2);
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
