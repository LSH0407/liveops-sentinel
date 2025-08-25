#include "NetworkDiagnostics.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace net {

NetworkDiagnostics::NetworkDiagnostics() 
    : latency_threshold_ms_(100.0)
    , packet_loss_threshold_pct_(5.0)
    , bandwidth_threshold_mbps_(10.0)
    , advanced_metrics_enabled_(true) {
    
    // 기본 타겟 설정
    default_targets_ = {"8.8.8.8", "1.1.1.1", "208.67.222.222"};
}

NetworkDiagnostics::~NetworkDiagnostics() = default;

std::vector<PingResult> NetworkDiagnostics::pingTest(const std::vector<std::string>& targets, int count) {
    std::vector<PingResult> results;
    
    for (const auto& target : targets) {
        if (progress_callback_) {
            progress_callback_(0, "Ping 테스트 시작: " + target);
        }
        
        for (int i = 0; i < count; ++i) {
            PingResult result;
            result.target = target;
            result.timestamp = std::chrono::system_clock::now();
            
            // 실제 ping 구현 (간단한 시뮬레이션)
            // 실제로는 ICMP ping 또는 TCP ping 구현 필요
            auto start = std::chrono::high_resolution_clock::now();
            
            // 네트워크 지연 시뮬레이션
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<> dist(20.0, 5.0); // 평균 20ms, 표준편차 5ms
            
            double simulated_rtt = dist(gen);
            if (simulated_rtt < 0) simulated_rtt = 5.0; // 최소 5ms
            
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(simulated_rtt)));
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            result.rtt_ms = duration.count() / 1000.0;
            result.success = true;
            
            results.push_back(result);
            
            if (progress_callback_) {
                int progress = ((i + 1) * 100) / count;
                progress_callback_(progress, "Ping 진행 중: " + target);
            }
        }
    }
    
    return results;
}

BandwidthTest NetworkDiagnostics::bandwidthTest(const std::string& target, int duration_seconds) {
    BandwidthTest test;
    test.target = target;
    test.timestamp = std::chrono::system_clock::now();
    
    if (progress_callback_) {
        progress_callback_(0, "대역폭 테스트 시작: " + target);
    }
    
    // 대역폭 테스트 시뮬레이션
    // 실제로는 HTTP 다운로드/업로드 테스트 구현 필요
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 다운로드 속도 시뮬레이션 (50-200 Mbps)
    std::uniform_real_distribution<> download_dist(50.0, 200.0);
    test.download_mbps = download_dist(gen);
    
    // 업로드 속도 시뮬레이션 (10-50 Mbps)
    std::uniform_real_distribution<> upload_dist(10.0, 50.0);
    test.upload_mbps = upload_dist(gen);
    
    // 지연 시간 시뮬레이션
    std::normal_distribution<> latency_dist(20.0, 5.0);
    test.latency_ms = latency_dist(gen);
    if (test.latency_ms < 5.0) test.latency_ms = 5.0;
    
    // 패킷 손실 시뮬레이션 (0-2%)
    std::uniform_real_distribution<> loss_dist(0.0, 2.0);
    test.packet_loss_pct = loss_dist(gen);
    
    if (progress_callback_) {
        progress_callback_(100, "대역폭 테스트 완료: " + target);
    }
    
    return test;
}

double NetworkDiagnostics::measurePacketLoss(const std::string& target, int packet_count) {
    if (progress_callback_) {
        progress_callback_(0, "패킷 손실 측정 시작: " + target);
    }
    
    // 패킷 손실 측정 시뮬레이션
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> loss_dist(0.0, 5.0);
    
    double loss_rate = loss_dist(gen);
    
    if (progress_callback_) {
        progress_callback_(100, "패킷 손실 측정 완료: " + target);
    }
    
    return loss_rate;
}

NetworkQuality NetworkDiagnostics::assessNetworkQuality(const std::string& target) {
    NetworkQuality quality;
    
    // 네트워크 품질 평가를 위한 테스트 실행
    auto ping_results = pingTest({target}, 10);
    auto bandwidth_test = bandwidthTest(target, 30);
    double packet_loss = measurePacketLoss(target, 100);
    
    // 지연 점수 계산 (0-100)
    if (ping_results.empty()) {
        quality.latency_score = 0.0;
    } else {
        double avg_latency = 0.0;
        for (const auto& result : ping_results) {
            avg_latency += result.rtt_ms;
        }
        avg_latency /= ping_results.size();
        
        // 지연 시간이 낮을수록 높은 점수
        if (avg_latency <= 20.0) quality.latency_score = 100.0;
        else if (avg_latency <= 50.0) quality.latency_score = 80.0;
        else if (avg_latency <= 100.0) quality.latency_score = 60.0;
        else if (avg_latency <= 200.0) quality.latency_score = 40.0;
        else quality.latency_score = 20.0;
    }
    
    // 대역폭 점수 계산 (0-100)
    if (bandwidth_test.download_mbps >= 100.0) quality.bandwidth_score = 100.0;
    else if (bandwidth_test.download_mbps >= 50.0) quality.bandwidth_score = 80.0;
    else if (bandwidth_test.download_mbps >= 25.0) quality.bandwidth_score = 60.0;
    else if (bandwidth_test.download_mbps >= 10.0) quality.bandwidth_score = 40.0;
    else quality.bandwidth_score = 20.0;
    
    // 안정성 점수 계산 (0-100)
    if (packet_loss <= 0.1) quality.stability_score = 100.0;
    else if (packet_loss <= 0.5) quality.stability_score = 80.0;
    else if (packet_loss <= 1.0) quality.stability_score = 60.0;
    else if (packet_loss <= 2.0) quality.stability_score = 40.0;
    else quality.stability_score = 20.0;
    
    // 전체 점수 계산 (가중 평균)
    quality.overall_score = (quality.latency_score * 0.4 + 
                            quality.bandwidth_score * 0.4 + 
                            quality.stability_score * 0.2);
    
    // 등급 계산
    quality.grade = calculateGrade(quality.overall_score);
    
    return quality;
}

NetworkPrediction NetworkDiagnostics::predictBandwidthUsage(const std::string& target) {
    NetworkPrediction prediction;
    
    // 대역폭 사용량 예측 시뮬레이션
    // 실제로는 과거 데이터를 기반으로 한 머신러닝 모델 사용
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> bandwidth_dist(50.0, 15.0);
    std::normal_distribution<> latency_dist(25.0, 8.0);
    std::uniform_real_distribution<> confidence_dist(0.7, 0.95);
    
    prediction.predicted_bandwidth_mbps = bandwidth_dist(gen);
    prediction.predicted_latency_ms = latency_dist(gen);
    prediction.confidence_level = confidence_dist(gen);
    prediction.prediction_horizon = std::chrono::minutes(30);
    
    // 트렌드 예측
    std::uniform_int_distribution<> trend_dist(0, 2);
    int trend = trend_dist(gen);
    switch (trend) {
        case 0: prediction.trend = "improving"; break;
        case 1: prediction.trend = "stable"; break;
        case 2: prediction.trend = "degrading"; break;
    }
    
    return prediction;
}

std::vector<std::string> NetworkDiagnostics::diagnoseNetworkIssues(const std::string& target) {
    std::vector<std::string> issues;
    
    // 네트워크 문제 진단
    auto ping_results = pingTest({target}, 5);
    auto bandwidth_test = bandwidthTest(target, 10);
    double packet_loss = measurePacketLoss(target, 50);
    
    // 고지연 문제 진단
    if (!ping_results.empty()) {
        double avg_latency = 0.0;
        for (const auto& result : ping_results) {
            avg_latency += result.rtt_ms;
        }
        avg_latency /= ping_results.size();
        
        if (avg_latency > latency_threshold_ms_) {
            issues.push_back("고지연 문제: 평균 지연시간 " + std::to_string(avg_latency) + "ms");
        }
    }
    
    // 패킷 손실 문제 진단
    if (packet_loss > packet_loss_threshold_pct_) {
        issues.push_back("패킷 손실 문제: 손실률 " + std::to_string(packet_loss) + "%");
    }
    
    // 대역폭 병목 문제 진단
    if (bandwidth_test.download_mbps < bandwidth_threshold_mbps_) {
        issues.push_back("대역폭 병목: 다운로드 속도 " + std::to_string(bandwidth_test.download_mbps) + "Mbps");
    }
    
    // 네트워크 불안정성 진단
    if (!isNetworkStable(ping_results)) {
        issues.push_back("네트워크 불안정성: 지연시간 변동이 큽니다");
    }
    
    return issues;
}

NetworkDiagnostics::LatencyAnalysis NetworkDiagnostics::analyzeLatency(const std::vector<PingResult>& ping_results) {
    LatencyAnalysis analysis;
    
    if (ping_results.empty()) {
        return analysis;
    }
    
    std::vector<double> latencies;
    for (const auto& result : ping_results) {
        if (result.success) {
            latencies.push_back(result.rtt_ms);
        }
    }
    
    if (latencies.empty()) {
        return analysis;
    }
    
    // 기본 통계 계산
    analysis.min_latency = *std::min_element(latencies.begin(), latencies.end());
    analysis.max_latency = *std::max_element(latencies.begin(), latencies.end());
    
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    analysis.avg_latency = sum / latencies.size();
    
    // 지터 계산
    analysis.jitter = calculateJitter(latencies);
    
    // 표준편차 계산
    analysis.std_deviation = calculateStandardDeviation(latencies);
    
    // 지연 시간 히스토리 저장
    analysis.latency_history = latencies;
    
    return analysis;
}

NetworkDiagnostics::BandwidthUsage NetworkDiagnostics::getBandwidthUsage(const std::string& interface) {
    BandwidthUsage usage;
    usage.timestamp = std::chrono::system_clock::now();
    
    // 대역폭 사용량 시뮬레이션
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> current_dist(30.0, 10.0);
    std::normal_distribution<> peak_dist(80.0, 20.0);
    std::normal_distribution<> avg_dist(25.0, 8.0);
    
    usage.current_usage_mbps = current_dist(gen);
    usage.peak_usage_mbps = peak_dist(gen);
    usage.average_usage_mbps = avg_dist(gen);
    
    // 히스토리 생성 (최근 10개 데이터)
    for (int i = 0; i < 10; ++i) {
        std::normal_distribution<> hist_dist(25.0, 8.0);
        usage.usage_history.push_back(hist_dist(gen));
    }
    
    return usage;
}

NetworkPrediction NetworkDiagnostics::predictBandwidthTrend(const std::vector<BandwidthUsage>& history) {
    NetworkPrediction prediction;
    
    if (history.size() < 2) {
        return prediction;
    }
    
    // 선형 회귀를 사용한 트렌드 예측
    std::vector<double> x, y;
    for (size_t i = 0; i < history.size(); ++i) {
        x.push_back(static_cast<double>(i));
        y.push_back(history[i].current_usage_mbps);
    }
    
    double slope = linearRegression(x, y);
    
    // 예측값 계산
    prediction.predicted_bandwidth_mbps = history.back().current_usage_mbps + slope * 5.0; // 5단계 후 예측
    prediction.predicted_latency_ms = 25.0; // 기본값
    prediction.confidence_level = 0.8;
    prediction.prediction_horizon = std::chrono::minutes(30);
    
    // 트렌드 결정
    if (slope > 1.0) prediction.trend = "improving";
    else if (slope < -1.0) prediction.trend = "degrading";
    else prediction.trend = "stable";
    
    return prediction;
}

std::vector<NetworkDiagnostics::NetworkIssue> NetworkDiagnostics::diagnoseIssues(const std::string& target) {
    std::vector<NetworkIssue> issues;
    
    // 지연 분석
    auto ping_results = pingTest({target}, 10);
    auto latency_analysis = analyzeLatency(ping_results);
    
    if (detectHighLatency(latency_analysis)) {
        NetworkIssue issue;
        issue.type = "high_latency";
        issue.severity = latency_analysis.avg_latency > 200.0 ? "critical" : "high";
        issue.description = "평균 지연시간이 " + std::to_string(latency_analysis.avg_latency) + "ms로 높습니다";
        issue.recommendations = {"네트워크 연결 확인", "ISP에 문의", "다른 서버 시도"};
        issue.confidence = 0.9;
        issues.push_back(issue);
    }
    
    // 패킷 손실 진단
    double packet_loss = measurePacketLoss(target, 100);
    if (detectPacketLoss(packet_loss)) {
        NetworkIssue issue;
        issue.type = "packet_loss";
        issue.severity = packet_loss > 10.0 ? "critical" : "medium";
        issue.description = "패킷 손실률이 " + std::to_string(packet_loss) + "%입니다";
        issue.recommendations = {"네트워크 케이블 확인", "라우터 재시작", "ISP에 문의"};
        issue.confidence = 0.85;
        issues.push_back(issue);
    }
    
    // 대역폭 병목 진단
    auto bandwidth_usage = getBandwidthUsage();
    if (detectBandwidthBottleneck(bandwidth_usage)) {
        NetworkIssue issue;
        issue.type = "bandwidth_bottleneck";
        issue.severity = "medium";
        issue.description = "대역폭 사용량이 높습니다: " + std::to_string(bandwidth_usage.current_usage_mbps) + "Mbps";
        issue.recommendations = {"불필요한 애플리케이션 종료", "대역폭 사용량 모니터링", "네트워크 업그레이드 고려"};
        issue.confidence = 0.75;
        issues.push_back(issue);
    }
    
    // 네트워크 불안정성 진단
    if (detectNetworkInstability(ping_results)) {
        NetworkIssue issue;
        issue.type = "network_instability";
        issue.severity = "high";
        issue.description = "네트워크 연결이 불안정합니다";
        issue.recommendations = {"네트워크 설정 확인", "라우터 재시작", "ISP에 문의"};
        issue.confidence = 0.8;
        issues.push_back(issue);
    }
    
    return issues;
}

void NetworkDiagnostics::setTargets(const std::vector<std::string>& targets) {
    default_targets_ = targets;
}

void NetworkDiagnostics::setThresholds(double latency_threshold_ms, double packet_loss_threshold_pct, 
                                      double bandwidth_threshold_mbps) {
    latency_threshold_ms_ = latency_threshold_ms;
    packet_loss_threshold_pct_ = packet_loss_threshold_pct;
    bandwidth_threshold_mbps_ = bandwidth_threshold_mbps;
}

void NetworkDiagnostics::enableAdvancedMetrics(bool enabled) {
    advanced_metrics_enabled_ = enabled;
}

json NetworkDiagnostics::saveDiagnosticsData() const {
    json data;
    data["targets"] = default_targets_;
    data["thresholds"] = {
        {"latency_ms", latency_threshold_ms_},
        {"packet_loss_pct", packet_loss_threshold_pct_},
        {"bandwidth_mbps", bandwidth_threshold_mbps_}
    };
    data["advanced_metrics"] = advanced_metrics_enabled_;
    return data;
}

void NetworkDiagnostics::loadDiagnosticsData(const json& data) {
    if (data.contains("targets") && data["targets"].is_array()) {
        default_targets_.clear();
        for (const auto& target : data["targets"]) {
            default_targets_.push_back(target.get<std::string>());
        }
    }
    
    if (data.contains("thresholds")) {
        auto& thresholds = data["thresholds"];
        latency_threshold_ms_ = thresholds.value("latency_ms", 100.0);
        packet_loss_threshold_pct_ = thresholds.value("packet_loss_pct", 5.0);
        bandwidth_threshold_mbps_ = thresholds.value("bandwidth_mbps", 10.0);
    }
    
    advanced_metrics_enabled_ = data.value("advanced_metrics", true);
}

void NetworkDiagnostics::setProgressCallback(ProgressCallback callback) {
    progress_callback_ = callback;
}

// 내부 헬퍼 함수들
double NetworkDiagnostics::calculateJitter(const std::vector<double>& latencies) {
    if (latencies.size() < 2) return 0.0;
    
    double jitter = 0.0;
    for (size_t i = 1; i < latencies.size(); ++i) {
        jitter += std::abs(latencies[i] - latencies[i-1]);
    }
    return jitter / (latencies.size() - 1);
}

double NetworkDiagnostics::calculateStandardDeviation(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0.0;
    
    for (double value : values) {
        variance += std::pow(value - mean, 2);
    }
    variance /= values.size();
    
    return std::sqrt(variance);
}

std::string NetworkDiagnostics::calculateGrade(double score) {
    if (score >= 90.0) return "A";
    else if (score >= 80.0) return "B";
    else if (score >= 70.0) return "C";
    else if (score >= 60.0) return "D";
    else return "F";
}

bool NetworkDiagnostics::isNetworkStable(const std::vector<PingResult>& results) {
    if (results.size() < 3) return true;
    
    std::vector<double> latencies;
    for (const auto& result : results) {
        if (result.success) {
            latencies.push_back(result.rtt_ms);
        }
    }
    
    if (latencies.size() < 3) return true;
    
    double std_dev = calculateStandardDeviation(latencies);
    double mean = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    
    // 변동계수 (CV) 계산: 표준편차 / 평균
    double cv = std_dev / mean;
    
    // CV가 0.3 이상이면 불안정으로 판단
    return cv < 0.3;
}

std::vector<double> NetworkDiagnostics::smoothData(const std::vector<double>& data, int window_size) {
    if (data.size() < window_size) return data;
    
    std::vector<double> smoothed;
    for (size_t i = 0; i <= data.size() - window_size; ++i) {
        double sum = 0.0;
        for (int j = 0; j < window_size; ++j) {
            sum += data[i + j];
        }
        smoothed.push_back(sum / window_size);
    }
    
    return smoothed;
}

double NetworkDiagnostics::linearRegression(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 2) return 0.0;
    
    double n = static_cast<double>(x.size());
    double sum_x = std::accumulate(x.begin(), x.end(), 0.0);
    double sum_y = std::accumulate(y.begin(), y.end(), 0.0);
    double sum_xy = 0.0;
    double sum_x2 = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        sum_xy += x[i] * y[i];
        sum_x2 += x[i] * x[i];
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    return slope;
}

double NetworkDiagnostics::exponentialSmoothing(const std::vector<double>& data, double alpha) {
    if (data.empty()) return 0.0;
    
    double smoothed = data[0];
    for (size_t i = 1; i < data.size(); ++i) {
        smoothed = alpha * data[i] + (1 - alpha) * smoothed;
    }
    
    return smoothed;
}

bool NetworkDiagnostics::detectHighLatency(const LatencyAnalysis& analysis) {
    return analysis.avg_latency > latency_threshold_ms_;
}

bool NetworkDiagnostics::detectPacketLoss(double loss_rate) {
    return loss_rate > packet_loss_threshold_pct_;
}

bool NetworkDiagnostics::detectBandwidthBottleneck(const BandwidthUsage& usage) {
    return usage.current_usage_mbps > bandwidth_threshold_mbps_ * 0.8; // 80% 이상 사용 시
}

bool NetworkDiagnostics::detectNetworkInstability(const std::vector<PingResult>& results) {
    return !isNetworkStable(results);
}

} // namespace net
