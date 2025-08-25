#pragma once
#include <string>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace core {

class PerformanceManager {
public:
    static PerformanceManager& getInstance();
    
    // 성능 보고서 구조체
    struct PerformanceReport {
        double cpu_usage_percent = 0.0;
        double memory_usage_percent = 0.0;
        double gpu_usage_percent = 0.0;
        double disk_usage_percent = 0.0;
        double network_usage_mbps = 0.0;
        size_t memory_usage_mb = 0;
        std::chrono::system_clock::time_point timestamp;
        
        // 시스템 상태 평가
        std::string overall_status; // "good", "warning", "critical"
        std::vector<std::string> recommendations;
    };
    
    PerformanceManager();
    ~PerformanceManager();
    
    // 초기화 및 종료
    void initialize();
    void shutdown();
    
    // 모니터링 제어
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // 성능 보고서
    PerformanceReport getPerformanceReport() const;
    std::string getStatusSummary() const;
    
    // 설정
    void setMonitoringInterval(std::chrono::seconds interval);
    void setWarningThresholds(double cpu_warning, double memory_warning, double gpu_warning);
    void setCriticalThresholds(double cpu_critical, double memory_critical, double gpu_critical);
    
    // 알림 콜백
    using AlertCallback = std::function<void(const std::string&, const std::string&)>;
    void setAlertCallback(AlertCallback callback);
    
private:
    // 모니터링 루프
    void monitoringLoop();
    void generatePerformanceReport();
    void evaluateSystemStatus();
    void generateRecommendations();
    
    // 멤버 변수
    std::atomic<bool> monitoring_enabled_{false};
    std::atomic<bool> running_{false};
    std::thread monitoring_thread_;
    std::chrono::seconds monitoring_interval_{5};
    
    // 임계값
    double cpu_warning_threshold_ = 80.0;
    double memory_warning_threshold_ = 80.0;
    double gpu_warning_threshold_ = 85.0;
    double cpu_critical_threshold_ = 95.0;
    double memory_critical_threshold_ = 95.0;
    double gpu_critical_threshold_ = 95.0;
    
    // 상태 관리
    mutable std::mutex report_mutex_;
    PerformanceReport current_report_;
    AlertCallback alert_callback_;
    
    // 이전 상태 (중복 알림 방지)
    std::string last_alert_status_;
    std::chrono::system_clock::time_point last_alert_time_;
};

} // namespace core
