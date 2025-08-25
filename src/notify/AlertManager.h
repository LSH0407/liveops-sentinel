#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class AlertLevel {
    INFO,
    WARNING,
    CRITICAL
};

struct Alert {
    AlertLevel level;
    std::string title;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::string source; // "network", "system", "obs"
    json metadata;
};

struct AlertThresholds {
    double rtt_ms_warning{80.0};
    double rtt_ms_critical{150.0};
    double loss_pct_warning{2.0};
    double loss_pct_critical{5.0};
    double cpu_pct_warning{80.0};
    double cpu_pct_critical{95.0};
    double gpu_pct_warning{85.0};
    double gpu_pct_critical{95.0};
    double dropped_ratio_warning{0.03};
    double dropped_ratio_critical{0.08};
    int hold_seconds{5}; // 연속 초과 시에만 알림
};

class AlertManager {
public:
    AlertManager();
    ~AlertManager();
    
    // 임계값 설정
    void setThresholds(const AlertThresholds& thresholds);
    AlertThresholds getThresholds() const;
    
    // 알림 콜백 설정
    void setAlertCallback(std::function<void(const Alert&)> callback);
    
    // 메트릭 업데이트 및 알림 생성
    void updateMetrics(const json& metrics);
    
    // 최근 알림 조회
    std::vector<Alert> getRecentAlerts(int count = 10) const;
    
    // 알림 통계
    int getAlertCount(AlertLevel level, std::chrono::minutes duration = std::chrono::minutes(60)) const;
    
    // 알림 초기화
    void clearAlerts();
    
private:
    AlertThresholds thresholds_;
    std::function<void(const Alert&)> alert_callback_;
    std::vector<Alert> recent_alerts_;
    
    // 연속 초과 카운터
    struct ViolationCounter {
        int rtt_count{0};
        int loss_count{0};
        int cpu_count{0};
        int gpu_count{0};
        int dropped_count{0};
        std::chrono::steady_clock::time_point last_reset{std::chrono::steady_clock::now()};
    } violation_counter_;
    
    // 알림 생성 헬퍼
    void createAlert(AlertLevel level, const std::string& title, const std::string& message, 
                    const std::string& source, const json& metadata = {});
    
    // 임계값 체크
    void checkNetworkThresholds(const json& metrics);
    void checkSystemThresholds(const json& metrics);
    void checkObsThresholds(const json& metrics);
    
    // 알림 중복 방지
    bool isDuplicateAlert(const Alert& alert) const;
};

