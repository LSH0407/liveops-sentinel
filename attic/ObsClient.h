#pragma once
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ObsStats {
    bool streaming{false};
    bool recording{false};
    double dropped_frames{0};
    double encoding_lag_ms{0};
    double render_lag_ms{0};
    std::string current_scene;
};

class ObsClient {
public:
    ObsClient();
    ~ObsClient();
    
    // 연결 관리
    bool connect(const std::string& url = "ws://localhost:4444", const std::string& password = "");
    void disconnect();
    bool isConnected() const;
    
    // 스트림 제어
    bool startStreaming();
    bool stopStreaming();
    bool startRecording();
    bool stopRecording();
    
    // 씬 관리
    bool setCurrentScene(const std::string& sceneName);
    std::string getCurrentScene();
    
    // 통계 수집
    ObsStats getStats();

private:
    class ObsClientImpl;
    std::unique_ptr<ObsClientImpl> impl_;
};
