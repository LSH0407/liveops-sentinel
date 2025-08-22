#pragma once
#include <string>
#include <functional>
#include <memory>

// OBS WebSocket functionality disabled due to websocketpp removal from vcpkg
// This is a stub implementation for console application

struct ObsStatus {
    bool connected{false};
    bool recording{false};
    bool streaming{false};
    std::string currentScene;
    std::string currentProgramScene;
    
    // 성능 지표
    double droppedFramesRatio{0.0};
    double outputSkippedFrames{0.0};
    double averageFrameRenderTimeMs{0.0};
    double cpuUsage{0.0};
    double memoryUsageMB{0.0};
    double activeFps{0.0};
    int64_t outputBytes{0};
    double strain{0.0};
    double encodingLagMs{0.0};
    double renderLagMs{0.0};
    
    // 비디오 설정
    int baseWidth{0};
    int baseHeight{0};
    int outputWidth{0};
    int outputHeight{0};
    double fps{0.0};
};

class ObsClient {
public:
    using StatusCallback = std::function<void(const ObsStatus&)>;
    
    ObsClient();
    ~ObsClient();
    
    bool connect(const std::string& host = "localhost", int port = 4455, 
                 const std::string& password = "");
    void disconnect();
    bool isConnected() const;
    
    void setStatusCallback(StatusCallback cb);
    ObsStatus getStatus() const;
    
    // OBS Control methods
    bool getSceneList(std::vector<std::string>& scenes);
    bool setCurrentProgramScene(const std::string& sceneName);
    bool startStream();
    bool stopStream();
    bool startRecord();
    bool stopRecord();
    bool getInputList(std::vector<std::string>& inputs);
    
    // Video Settings
    bool getVideoSettings();
    
    // Event subscription
    void subscribeToEvents();
    void setEventCallback(std::function<void(const std::string&, const std::string&)> cb);
    
private:
    // OBS WebSocket functionality disabled - stub methods
    void sendRequest(const std::string& requestType, const std::string& data = "{}");
    void handleResponse(const std::string& response);
    void updateStatus();
    void requestStats();
    
    std::string host_;
    int port_;
    std::string password_;
    bool connected_{false};
    StatusCallback statusCallback_;
    ObsStatus status_;
    
    // OBS WebSocket API 요청 ID
    int requestId_{1};
    
    // Event handling
    std::function<void(const std::string&, const std::string&)> eventCallback_;
    bool eventsSubscribed_{false};
};
