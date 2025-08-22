#pragma once
#include <string>
#include <functional>
#include <memory>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

using WebSocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WebSocketConnection = WebSocketClient::connection_ptr;

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
    void onOpen(websocketpp::connection_hdl hdl);
    void onClose(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg);
    void onError(websocketpp::connection_hdl hdl);
    
    void sendRequest(const std::string& requestType, const std::string& data = "{}");
    void handleResponse(const std::string& response);
    void updateStatus();
    void requestStats();
    
    std::unique_ptr<WebSocketClient> client_;
    websocketpp::connection_hdl connection_;
    std::string host_;
    int port_;
    std::string password_;
    bool connected_{false};
    StatusCallback statusCallback_;
    ObsStatus status_;
    
    // OBS WebSocket API 요청 ID
    int requestId_{1};
    
    // 주기적 통계 수집
    std::thread stats_thread_;
    std::atomic<bool> stats_running_{false};
    std::chrono::steady_clock::time_point last_stats_request_;
    
    // Event handling
    std::function<void(const std::string&, const std::string&)> eventCallback_;
    bool eventsSubscribed_{false};
};
