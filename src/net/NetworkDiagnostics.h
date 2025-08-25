#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace net {

struct PingResult {
    std::string target;
    double rtt_ms;
    bool success;
    std::chrono::system_clock::time_point timestamp;
    
    PingResult() : rtt_ms(0.0), success(false) {}
};

struct BandwidthTest {
    std::string target;
    double upload_mbps;
    double download_mbps;
    double latency_ms;
    double packet_loss_pct;
    std::chrono::system_clock::time_point timestamp;
    
    BandwidthTest() : upload_mbps(0.0), download_mbps(0.0), 
                     latency_ms(0.0), packet_loss_pct(0.0) {}
};

struct NetworkQuality {
    double overall_score;      // 0-100 점수
    double latency_score;      // 지연 점수
    double bandwidth_score;    // 대역폭 점수
    double stability_score;    // 안정성 점수
    std::string grade;         // A, B, C, D, F 등급
    
    NetworkQuality() : overall_score(0.0), latency_score(0.0), 
                      bandwidth_score(0.0), stability_score(0.0) {}
};

struct NetworkPrediction {
    double predicted_bandwidth_mbps;
    double predicted_latency_ms;
    double confidence_level;   // 0-1 신뢰도
    std::chrono::minutes prediction_horizon;
    std::string trend;         // "improving", "stable", "degrading"
    
    NetworkPrediction() : predicted_bandwidth_mbps(0.0), predicted_latency_ms(0.0),
                         confidence_level(0.0), prediction_horizon(std::chrono::minutes(0)) {}
};

class NetworkDiagnostics {
public:
    NetworkDiagnostics();
    ~NetworkDiagnostics();
    
    // 기본 진단 기능
    std::vector<PingResult> pingTest(const std::vector<std::string>& targets, int count = 5);
    BandwidthTest bandwidthTest(const std::string& target, int duration_seconds = 30);
    double measurePacketLoss(const std::string& target, int packet_count = 100);
    
    // 고급 진단 기능
    NetworkQuality assessNetworkQuality(const std::string& target);
    NetworkPrediction predictBandwidthUsage(const std::string& target);
    std::vector<std::string> diagnoseNetworkIssues(const std::string& target);
    
    // 지연 분석
    struct LatencyAnalysis {
        double min_latency;
        double max_latency;
        double avg_latency;
        double jitter;         // 지터 (변동성)
        double std_deviation;  // 표준편차
        std::vector<double> latency_history;
    };
    
    LatencyAnalysis analyzeLatency(const std::vector<PingResult>& ping_results);
    
    // 대역폭 사용량 예측
    struct BandwidthUsage {
        double current_usage_mbps;
        double peak_usage_mbps;
        double average_usage_mbps;
        std::chrono::system_clock::time_point timestamp;
        std::vector<double> usage_history;
    };
    
    BandwidthUsage getBandwidthUsage(const std::string& interface = "");
    NetworkPrediction predictBandwidthTrend(const std::vector<BandwidthUsage>& history);
    
    // 네트워크 문제 진단
    struct NetworkIssue {
        std::string type;          // "high_latency", "packet_loss", "bandwidth_bottleneck", etc.
        std::string severity;      // "low", "medium", "high", "critical"
        std::string description;
        std::vector<std::string> recommendations;
        double confidence;         // 0-1 신뢰도
    };
    
    std::vector<NetworkIssue> diagnoseIssues(const std::string& target);
    
    // 설정 관리
    void setTargets(const std::vector<std::string>& targets);
    void setThresholds(double latency_threshold_ms, double packet_loss_threshold_pct, 
                      double bandwidth_threshold_mbps);
    void enableAdvancedMetrics(bool enabled);
    
    // 결과 저장/로드
    json saveDiagnosticsData() const;
    void loadDiagnosticsData(const json& data);
    
    // 콜백 설정
    using ProgressCallback = std::function<void(int progress, const std::string& status)>;
    void setProgressCallback(ProgressCallback callback);
    
private:
    std::vector<std::string> default_targets_;
    double latency_threshold_ms_;
    double packet_loss_threshold_pct_;
    double bandwidth_threshold_mbps_;
    bool advanced_metrics_enabled_;
    ProgressCallback progress_callback_;
    
    // 내부 헬퍼 함수들
    double calculateJitter(const std::vector<double>& latencies);
    double calculateStandardDeviation(const std::vector<double>& values);
    std::string calculateGrade(double score);
    bool isNetworkStable(const std::vector<PingResult>& results);
    std::vector<double> smoothData(const std::vector<double>& data, int window_size = 5);
    
    // 예측 알고리즘
    double linearRegression(const std::vector<double>& x, const std::vector<double>& y);
    double exponentialSmoothing(const std::vector<double>& data, double alpha = 0.3);
    
    // 문제 진단 로직
    bool detectHighLatency(const LatencyAnalysis& analysis);
    bool detectPacketLoss(double loss_rate);
    bool detectBandwidthBottleneck(const BandwidthUsage& usage);
    bool detectNetworkInstability(const std::vector<PingResult>& results);
};

} // namespace net
