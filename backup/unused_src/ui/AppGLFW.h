#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace ui {

struct DashboardData {
    // 네트워크 메트릭
    double rtt_ms{0.0};
    double loss_pct{0.0};
    double bandwidth_mbps{0.0};
    
    // 시스템 메트릭
    double cpu_pct{0.0};
    double gpu_pct{0.0};
    double memory_pct{0.0};
    
    // OBS 메트릭
    bool streaming{false};
    bool recording{false};
    double dropped_frames{0.0};
    double encoding_lag_ms{0.0};
    double render_lag_ms{0.0};
    std::string current_scene;
    
    // 알림 통계
    int alert_count_warning{0};
    int alert_count_critical{0};
};

struct AlertItem {
    std::string level;
    std::string title;
    std::string message;
    std::string timestamp;
    std::string source;
};

class AppGLFW {
public:
    AppGLFW();
    ~AppGLFW();
    
    // 애플리케이션 실행
    int run();
    
    // 데이터 업데이트
    void updateDashboardData(const DashboardData& data);
    void updateAlerts(const std::vector<AlertItem>& alerts);
    
    // 콜백 설정
    void setStartStreamingCallback(std::function<void()> callback);
    void setStopStreamingCallback(std::function<void()> callback);
    void setStartRecordingCallback(std::function<void()> callback);
    void setStopRecordingCallback(std::function<void()> callback);
    void setSceneChangeCallback(std::function<void(const std::string&)> callback);
    void setDiscordWebhookCallback(std::function<void(const std::string&)> callback);
    
private:
    class AppGLFWImpl;
    std::unique_ptr<AppGLFWImpl> impl_;
};

} // namespace ui
