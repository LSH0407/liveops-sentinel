#include "PerformanceManager.h"
#include "SystemMetrics.h"
#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace core {

PerformanceManager& PerformanceManager::getInstance() {
    static PerformanceManager instance;
    return instance;
}

PerformanceManager::PerformanceManager() 
    : monitoring_enabled_(false)
    , running_(false)
    , monitoring_interval_(std::chrono::seconds(5))
    , last_alert_status_("good")
    , last_alert_time_(std::chrono::system_clock::now()) {
}

PerformanceManager::~PerformanceManager() {
    shutdown();
}

void PerformanceManager::initialize() {
    if (running_) return;
    
    core::Logger::getInstance().info("성능 모니터링 시스템 초기화");
    running_ = true;
}

void PerformanceManager::shutdown() {
    if (!running_) return;
    
    core::Logger::getInstance().info("성능 모니터링 시스템 종료");
    stopMonitoring();
    running_ = false;
}

void PerformanceManager::startMonitoring() {
    if (monitoring_enabled_) return;
    
    monitoring_enabled_ = true;
    monitoring_thread_ = std::thread(&PerformanceManager::monitoringLoop, this);
    
    core::Logger::getInstance().info("성능 모니터링 시작");
}

void PerformanceManager::stopMonitoring() {
    if (!monitoring_enabled_) return;
    
    monitoring_enabled_ = false;
    
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    
    core::Logger::getInstance().info("성능 모니터링 중지");
}

bool PerformanceManager::isMonitoring() const {
    return monitoring_enabled_;
}

PerformanceManager::PerformanceReport PerformanceManager::getPerformanceReport() const {
    std::lock_guard<std::mutex> lock(report_mutex_);
    return current_report_;
}

std::string PerformanceManager::getStatusSummary() const {
    std::lock_guard<std::mutex> lock(report_mutex_);
    
    std::ostringstream oss;
    oss << "시스템 상태 요약:\n";
    oss << "  전체 상태: " << current_report_.overall_status << "\n";
    oss << "  CPU 사용률: " << std::fixed << std::setprecision(1) << current_report_.cpu_usage_percent << "%\n";
    oss << "  메모리 사용률: " << std::fixed << std::setprecision(1) << current_report_.memory_usage_percent << "%\n";
    oss << "  GPU 사용률: " << std::fixed << std::setprecision(1) << current_report_.gpu_usage_percent << "%\n";
    oss << "  디스크 사용률: " << std::fixed << std::setprecision(1) << current_report_.disk_usage_percent << "%\n";
    oss << "  네트워크 사용률: " << std::fixed << std::setprecision(1) << current_report_.network_usage_mbps << " Mbps\n";
    
    if (!current_report_.recommendations.empty()) {
        oss << "  권장사항:\n";
        for (const auto& rec : current_report_.recommendations) {
            oss << "    - " << rec << "\n";
        }
    }
    
    return oss.str();
}

void PerformanceManager::setMonitoringInterval(std::chrono::seconds interval) {
    monitoring_interval_ = interval;
}

void PerformanceManager::setWarningThresholds(double cpu_warning, double memory_warning, double gpu_warning) {
    cpu_warning_threshold_ = cpu_warning;
    memory_warning_threshold_ = memory_warning;
    gpu_warning_threshold_ = gpu_warning;
}

void PerformanceManager::setCriticalThresholds(double cpu_critical, double memory_critical, double gpu_critical) {
    cpu_critical_threshold_ = cpu_critical;
    memory_critical_threshold_ = memory_critical;
    gpu_critical_threshold_ = gpu_critical;
}

void PerformanceManager::setAlertCallback(AlertCallback callback) {
    alert_callback_ = callback;
}

void PerformanceManager::monitoringLoop() {
    while (monitoring_enabled_ && running_) {
        generatePerformanceReport();
        evaluateSystemStatus();
        
        std::this_thread::sleep_for(monitoring_interval_);
    }
}

void PerformanceManager::generatePerformanceReport() {
    std::lock_guard<std::mutex> lock(report_mutex_);
    
    // SystemMetrics를 사용하여 현재 시스템 상태 수집
    auto& system_metrics = SystemMetrics::getInstance();
    auto metrics = system_metrics.getMetrics();
    
    current_report_.cpu_usage_percent = metrics.value("cpu_pct", 0.0);
    current_report_.gpu_usage_percent = metrics.value("gpu_pct", 0.0);
    current_report_.memory_usage_percent = metrics.value("memory_pct", 0.0);
    current_report_.disk_usage_percent = metrics.value("disk_pct", 0.0);
    current_report_.network_usage_mbps = metrics.value("network_mbps", 0.0);
    current_report_.timestamp = std::chrono::system_clock::now();
    
    // 메모리 사용량을 MB로 변환 (대략적 계산)
    current_report_.memory_usage_mb = static_cast<size_t>(
        (current_report_.memory_usage_percent / 100.0) * 8192); // 8GB 기준
}

void PerformanceManager::evaluateSystemStatus() {
    std::lock_guard<std::mutex> lock(report_mutex_);
    
    std::string new_status = "good";
    std::vector<std::string> alerts;
    
    // CPU 상태 평가
    if (current_report_.cpu_usage_percent >= cpu_critical_threshold_) {
        new_status = "critical";
        alerts.push_back("CPU 사용률이 위험 수준입니다: " + 
                        std::to_string(static_cast<int>(current_report_.cpu_usage_percent)) + "%");
    } else if (current_report_.cpu_usage_percent >= cpu_warning_threshold_) {
        if (new_status != "critical") new_status = "warning";
        alerts.push_back("CPU 사용률이 높습니다: " + 
                        std::to_string(static_cast<int>(current_report_.cpu_usage_percent)) + "%");
    }
    
    // 메모리 상태 평가
    if (current_report_.memory_usage_percent >= memory_critical_threshold_) {
        new_status = "critical";
        alerts.push_back("메모리 사용률이 위험 수준입니다: " + 
                        std::to_string(static_cast<int>(current_report_.memory_usage_percent)) + "%");
    } else if (current_report_.memory_usage_percent >= memory_warning_threshold_) {
        if (new_status != "critical") new_status = "warning";
        alerts.push_back("메모리 사용률이 높습니다: " + 
                        std::to_string(static_cast<int>(current_report_.memory_usage_percent)) + "%");
    }
    
    // GPU 상태 평가
    if (current_report_.gpu_usage_percent >= gpu_critical_threshold_) {
        new_status = "critical";
        alerts.push_back("GPU 사용률이 위험 수준입니다: " + 
                        std::to_string(static_cast<int>(current_report_.gpu_usage_percent)) + "%");
    } else if (current_report_.gpu_usage_percent >= gpu_warning_threshold_) {
        if (new_status != "critical") new_status = "warning";
        alerts.push_back("GPU 사용률이 높습니다: " + 
                        std::to_string(static_cast<int>(current_report_.gpu_usage_percent)) + "%");
    }
    
    // 상태 변경 시 알림
    if (new_status != last_alert_status_) {
        last_alert_status_ = new_status;
        last_alert_time_ = std::chrono::system_clock::now();
        
        if (alert_callback_) {
            for (const auto& alert : alerts) {
                alert_callback_(new_status, alert);
            }
        }
        
        core::Logger::getInstance().warn("시스템 상태 변경: {} -> {}", last_alert_status_, new_status);
    }
    
    current_report_.overall_status = new_status;
    generateRecommendations();
}

void PerformanceManager::generateRecommendations() {
    current_report_.recommendations.clear();
    
    // CPU 권장사항
    if (current_report_.cpu_usage_percent > 90) {
        current_report_.recommendations.push_back("CPU 사용률이 매우 높습니다. 불필요한 프로그램을 종료하거나 OBS 설정을 낮춰보세요.");
    } else if (current_report_.cpu_usage_percent > 80) {
        current_report_.recommendations.push_back("CPU 사용률이 높습니다. 인코딩 설정을 확인해보세요.");
    }
    
    // 메모리 권장사항
    if (current_report_.memory_usage_percent > 90) {
        current_report_.recommendations.push_back("메모리 사용률이 매우 높습니다. 메모리 정리나 재부팅을 고려해보세요.");
    } else if (current_report_.memory_usage_percent > 80) {
        current_report_.recommendations.push_back("메모리 사용률이 높습니다. 불필요한 프로그램을 종료해보세요.");
    }
    
    // GPU 권장사항
    if (current_report_.gpu_usage_percent > 90) {
        current_report_.recommendations.push_back("GPU 사용률이 매우 높습니다. 그래픽 설정을 낮춰보세요.");
    } else if (current_report_.gpu_usage_percent > 80) {
        current_report_.recommendations.push_back("GPU 사용률이 높습니다. 게임이나 그래픽 프로그램을 확인해보세요.");
    }
    
    // 디스크 권장사항
    if (current_report_.disk_usage_percent > 90) {
        current_report_.recommendations.push_back("디스크 공간이 부족합니다. 불필요한 파일을 정리해보세요.");
    }
    
    // 네트워크 권장사항
    if (current_report_.network_usage_mbps > 100) {
        current_report_.recommendations.push_back("네트워크 사용량이 높습니다. 업로드 설정을 확인해보세요.");
    }
}

} // namespace core
