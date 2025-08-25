#include "AlertManager.h"
#include "../alert/Notifier.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

AlertManager::AlertManager() {
    // 기본 알림 콜백 설정 (Discord로 전송)
    setAlertCallback([](const Alert& alert) {
        std::string color;
        switch (alert.level) {
            case AlertLevel::INFO: color = "0x00ff00"; break;     // 녹색
            case AlertLevel::WARNING: color = "0xffff00"; break;  // 노란색
            case AlertLevel::CRITICAL: color = "0xff0000"; break; // 빨간색
        }
        
        std::string level_str;
        switch (alert.level) {
            case AlertLevel::INFO: level_str = "INFO"; break;
            case AlertLevel::WARNING: level_str = "WARNING"; break;
            case AlertLevel::CRITICAL: level_str = "CRITICAL"; break;
        }
        
        std::string title = "[" + level_str + "] " + alert.title;
        alert::sendDiscordEmbed(title, alert.message, color);
    });
}

AlertManager::~AlertManager() = default;

void AlertManager::setThresholds(const AlertThresholds& thresholds) {
    thresholds_ = thresholds;
}

AlertThresholds AlertManager::getThresholds() const {
    return thresholds_;
}

void AlertManager::setAlertCallback(std::function<void(const Alert&)> callback) {
    alert_callback_ = callback;
}

void AlertManager::updateMetrics(const json& metrics) {
    // 네트워크 임계값 체크
    checkNetworkThresholds(metrics);
    
    // 시스템 임계값 체크
    checkSystemThresholds(metrics);
    
    // OBS 임계값 체크
    checkObsThresholds(metrics);
    
    // 연속 초과 카운터 리셋 (hold_seconds 후)
    auto now = std::chrono::steady_clock::now();
    if (now - violation_counter_.last_reset > std::chrono::seconds(thresholds_.hold_seconds)) {
        violation_counter_.rtt_count = 0;
        violation_counter_.loss_count = 0;
        violation_counter_.cpu_count = 0;
        violation_counter_.gpu_count = 0;
        violation_counter_.dropped_count = 0;
        violation_counter_.last_reset = now;
    }
}

std::vector<Alert> AlertManager::getRecentAlerts(int count) const {
    std::vector<Alert> result;
    int start = std::max(0, static_cast<int>(recent_alerts_.size()) - count);
    
    for (int i = start; i < static_cast<int>(recent_alerts_.size()); ++i) {
        result.push_back(recent_alerts_[i]);
    }
    
    return result;
}

int AlertManager::getAlertCount(AlertLevel level, std::chrono::minutes duration) const {
    auto cutoff = std::chrono::system_clock::now() - duration;
    
    return std::count_if(recent_alerts_.begin(), recent_alerts_.end(),
        [level, cutoff](const Alert& alert) {
            return alert.level == level && alert.timestamp > cutoff;
        });
}

void AlertManager::clearAlerts() {
    recent_alerts_.clear();
}

void AlertManager::createAlert(AlertLevel level, const std::string& title, 
                              const std::string& message, const std::string& source, 
                              const json& metadata) {
    Alert alert;
    alert.level = level;
    alert.title = title;
    alert.message = message;
    alert.timestamp = std::chrono::system_clock::now();
    alert.source = source;
    alert.metadata = metadata;
    
    // 중복 알림 방지
    if (isDuplicateAlert(alert)) {
        return;
    }
    
    // 알림 목록에 추가
    recent_alerts_.push_back(alert);
    
    // 최대 100개까지만 유지
    if (recent_alerts_.size() > 100) {
        recent_alerts_.erase(recent_alerts_.begin());
    }
    
    // 콜백 호출
    if (alert_callback_) {
        alert_callback_(alert);
    }
    
    // 콘솔 출력
    std::string level_str;
    switch (level) {
        case AlertLevel::INFO: level_str = "INFO"; break;
        case AlertLevel::WARNING: level_str = "WARN"; break;
        case AlertLevel::CRITICAL: level_str = "CRIT"; break;
    }
    
    auto time_t = std::chrono::system_clock::to_time_t(alert.timestamp);
    std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
              << "] [" << level_str << "] " << title << ": " << message << std::endl;
}

void AlertManager::checkNetworkThresholds(const json& metrics) {
    if (!metrics.contains("network")) return;
    
    const auto& network = metrics["network"];
    
    // RTT 체크
    if (network.contains("rtt_ms")) {
        double rtt = network["rtt_ms"];
        
        if (rtt > thresholds_.rtt_ms_critical) {
            violation_counter_.rtt_count++;
            if (violation_counter_.rtt_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::CRITICAL, "Network RTT Critical", 
                           "RTT is " + std::to_string(static_cast<int>(rtt)) + "ms (threshold: " + 
                           std::to_string(static_cast<int>(thresholds_.rtt_ms_critical)) + "ms)", 
                           "network", {{"rtt_ms", rtt}});
            }
        } else if (rtt > thresholds_.rtt_ms_warning) {
            violation_counter_.rtt_count++;
            if (violation_counter_.rtt_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::WARNING, "Network RTT Warning", 
                           "RTT is " + std::to_string(static_cast<int>(rtt)) + "ms (threshold: " + 
                           std::to_string(static_cast<int>(thresholds_.rtt_ms_warning)) + "ms)", 
                           "network", {{"rtt_ms", rtt}});
            }
        } else {
            violation_counter_.rtt_count = 0;
        }
    }
    
    // 패킷 손실 체크
    if (network.contains("loss_pct")) {
        double loss = network["loss_pct"];
        
        if (loss > thresholds_.loss_pct_critical) {
            violation_counter_.loss_count++;
            if (violation_counter_.loss_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::CRITICAL, "Network Packet Loss Critical", 
                           "Packet loss is " + std::to_string(loss) + "% (threshold: " + 
                           std::to_string(thresholds_.loss_pct_critical) + "%)", 
                           "network", {{"loss_pct", loss}});
            }
        } else if (loss > thresholds_.loss_pct_warning) {
            violation_counter_.loss_count++;
            if (violation_counter_.loss_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::WARNING, "Network Packet Loss Warning", 
                           "Packet loss is " + std::to_string(loss) + "% (threshold: " + 
                           std::to_string(thresholds_.loss_pct_warning) + "%)", 
                           "network", {{"loss_pct", loss}});
            }
        } else {
            violation_counter_.loss_count = 0;
        }
    }
}

void AlertManager::checkSystemThresholds(const json& metrics) {
    if (!metrics.contains("system")) return;
    
    const auto& system = metrics["system"];
    
    // CPU 사용률 체크
    if (system.contains("cpu_pct")) {
        double cpu = system["cpu_pct"];
        
        if (cpu > thresholds_.cpu_pct_critical) {
            violation_counter_.cpu_count++;
            if (violation_counter_.cpu_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::CRITICAL, "CPU Usage Critical", 
                           "CPU usage is " + std::to_string(static_cast<int>(cpu)) + "% (threshold: " + 
                           std::to_string(static_cast<int>(thresholds_.cpu_pct_critical)) + "%)", 
                           "system", {{"cpu_pct", cpu}});
            }
        } else if (cpu > thresholds_.cpu_pct_warning) {
            violation_counter_.cpu_count++;
            if (violation_counter_.cpu_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::WARNING, "CPU Usage Warning", 
                           "CPU usage is " + std::to_string(static_cast<int>(cpu)) + "% (threshold: " + 
                           std::to_string(static_cast<int>(thresholds_.cpu_pct_warning)) + "%)", 
                           "system", {{"cpu_pct", cpu}});
            }
        } else {
            violation_counter_.cpu_count = 0;
        }
    }
    
    // GPU 사용률 체크
    if (system.contains("gpu_pct")) {
        double gpu = system["gpu_pct"];
        
        if (gpu > thresholds_.gpu_pct_critical) {
            violation_counter_.gpu_count++;
            if (violation_counter_.gpu_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::CRITICAL, "GPU Usage Critical", 
                           "GPU usage is " + std::to_string(static_cast<int>(gpu)) + "% (threshold: " + 
                           std::to_string(static_cast<int>(thresholds_.gpu_pct_critical)) + "%)", 
                           "system", {{"gpu_pct", gpu}});
            }
        } else if (gpu > thresholds_.gpu_pct_warning) {
            violation_counter_.gpu_count++;
            if (violation_counter_.gpu_count >= thresholds_.hold_seconds) {
                createAlert(AlertLevel::WARNING, "GPU Usage Warning", 
                           "GPU usage is " + std::to_string(static_cast<int>(gpu)) + "% (threshold: " + 
                           std::to_string(static_cast<int>(thresholds_.gpu_pct_warning)) + "%)", 
                           "system", {{"gpu_pct", gpu}});
            }
        } else {
            violation_counter_.gpu_count = 0;
        }
    }
}

void AlertManager::checkObsThresholds(const json& metrics) {
    if (!metrics.contains("obs")) return;
    
    const auto& obs = metrics["obs"];
    
    // 드롭된 프레임 비율 체크
    if (obs.contains("dropped_frames") && obs.contains("total_frames")) {
        double dropped = obs["dropped_frames"];
        double total = obs["total_frames"];
        
        if (total > 0) {
            double ratio = dropped / total;
            
            if (ratio > thresholds_.dropped_ratio_critical) {
                violation_counter_.dropped_count++;
                if (violation_counter_.dropped_count >= thresholds_.hold_seconds) {
                    createAlert(AlertLevel::CRITICAL, "OBS Frame Drop Critical", 
                               "Frame drop ratio is " + std::to_string(ratio * 100) + "% (threshold: " + 
                               std::to_string(thresholds_.dropped_ratio_critical * 100) + "%)", 
                               "obs", {{"dropped_ratio", ratio}});
                }
            } else if (ratio > thresholds_.dropped_ratio_warning) {
                violation_counter_.dropped_count++;
                if (violation_counter_.dropped_count >= thresholds_.hold_seconds) {
                    createAlert(AlertLevel::WARNING, "OBS Frame Drop Warning", 
                               "Frame drop ratio is " + std::to_string(ratio * 100) + "% (threshold: " + 
                               std::to_string(thresholds_.dropped_ratio_warning * 100) + "%)", 
                               "obs", {{"dropped_ratio", ratio}});
                }
            } else {
                violation_counter_.dropped_count = 0;
            }
        }
    }
    
    // 인코딩/렌더 랙 체크
    if (obs.contains("encoding_lag_ms")) {
        double encoding_lag = obs["encoding_lag_ms"];
        if (encoding_lag > 50.0) { // 50ms 이상이면 경고
            createAlert(AlertLevel::WARNING, "OBS Encoding Lag", 
                       "Encoding lag is " + std::to_string(encoding_lag) + "ms", 
                       "obs", {{"encoding_lag_ms", encoding_lag}});
        }
    }
    
    if (obs.contains("render_lag_ms")) {
        double render_lag = obs["render_lag_ms"];
        if (render_lag > 30.0) { // 30ms 이상이면 경고
            createAlert(AlertLevel::WARNING, "OBS Render Lag", 
                       "Render lag is " + std::to_string(render_lag) + "ms", 
                       "obs", {{"render_lag_ms", render_lag}});
        }
    }
}

bool AlertManager::isDuplicateAlert(const Alert& alert) const {
    auto now = std::chrono::system_clock::now();
    auto five_minutes_ago = now - std::chrono::minutes(5);
    
    // 최근 5분 내에 동일한 소스와 레벨의 알림이 있는지 확인
    for (const auto& existing : recent_alerts_) {
        if (existing.source == alert.source && 
            existing.level == alert.level && 
            existing.timestamp > five_minutes_ago) {
            return true;
        }
    }
    
    return false;
}

