#include "ObsClient.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <iostream>

using json = nlohmann::json;

ObsClient::ObsClient() {
    // OBS WebSocket functionality disabled - console stub
    spdlog::info("OBS Client initialized (WebSocket disabled)");
}

ObsClient::~ObsClient() {
    disconnect();
}

bool ObsClient::connect(const std::string& host, int port, const std::string& password) {
    // OBS WebSocket functionality disabled - console stub
    spdlog::info("OBS WebSocket connection disabled (console mode)");
    return false;
}

void ObsClient::disconnect() {
    // OBS WebSocket functionality disabled - console stub
    spdlog::info("OBS WebSocket disconnect (console mode)");
}

bool ObsClient::isConnected() const {
    return connected_;
}

void ObsClient::setStatusCallback(StatusCallback cb) {
    statusCallback_ = std::move(cb);
}

ObsStatus ObsClient::getStatus() const {
    return status_;
}

void ObsClient::onOpen(websocketpp::connection_hdl hdl) {
    spdlog::info("Connected to OBS WebSocket");
    connected_ = true;
    
    // OBS WebSocket API 인증 및 상태 요청
    if (!password_.empty()) {
        json authRequest = {
            {"requestType", "GetAuthRequired"},
            {"requestId", requestId_++}
        };
        sendRequest("GetAuthRequired");
    } else {
        updateStatus();
    }
}

void ObsClient::onClose(websocketpp::connection_hdl hdl) {
    spdlog::info("Disconnected from OBS WebSocket");
    connected_ = false;
    status_.connected = false;
    
    if (statusCallback_) {
        statusCallback_(status_);
    }
}

void ObsClient::onMessage(websocketpp::connection_hdl hdl, WebSocketClient::message_ptr msg) {
    try {
        json response = json::parse(msg->get_payload());
        handleResponse(response.dump());
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse OBS WebSocket message: {}", e.what());
    }
}

void ObsClient::onError(websocketpp::connection_hdl hdl) {
    spdlog::error("OBS WebSocket connection error");
    connected_ = false;
    status_.connected = false;
}

void ObsClient::sendRequest(const std::string& requestType, const std::string& data) {
    if (!connected_) return;
    
    try {
        json request = {
            {"requestType", requestType},
            {"requestId", requestId_++}
        };
        
        if (!data.empty() && data != "{}") {
            json dataJson = json::parse(data);
            request["requestData"] = dataJson;
        }
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        spdlog::error("Failed to send OBS request: {}", e.what());
    }
}

void ObsClient::handleResponse(const std::string& response) {
    try {
        json j = json::parse(response);
        
        if (j.contains("updateType")) {
            // 이벤트 업데이트
            std::string updateType = j["updateType"];
            if (updateType == "RecordingStarted" || updateType == "RecordingStopped") {
                status_.recording = (updateType == "RecordingStarted");
            } else if (updateType == "StreamingStarted" || updateType == "StreamingStopped") {
                status_.streaming = (updateType == "StreamingStarted");
            } else if (updateType == "SceneChanged") {
                if (j.contains("sceneName")) {
                    status_.currentScene = j["sceneName"];
                }
            }
            
            if (statusCallback_) {
                statusCallback_(status_);
            }
        } else if (j.contains("requestType")) {
            // 요청 응답
            std::string requestType = j["requestType"];
            if (requestType == "GetSceneList") {
                if (j.contains("responseData") && j["responseData"].contains("currentProgramSceneName")) {
                    status_.currentProgramScene = j["responseData"]["currentProgramSceneName"];
                }
            } else if (requestType == "GetRecordStatus") {
                if (j.contains("responseData") && j["responseData"].contains("outputActive")) {
                    status_.recording = j["responseData"]["outputActive"];
                }
            } else if (requestType == "GetStreamStatus") {
                if (j.contains("responseData") && j["responseData"].contains("outputActive")) {
                    status_.streaming = j["responseData"]["outputActive"];
                }
            } else if (requestType == "GetStats") {
                if (j.contains("responseData")) {
                    auto& stats = j["responseData"];
                    if (stats.contains("droppedFrames")) {
                        status_.droppedFramesRatio = stats["droppedFrames"].get<double>() / 100.0;
                    }
                    if (stats.contains("averageFrameRenderTime")) {
                        status_.averageFrameRenderTimeMs = stats["averageFrameRenderTime"].get<double>();
                    }
                    if (stats.contains("cpuUsage")) {
                        status_.cpuUsage = stats["cpuUsage"].get<double>();
                    }
                    if (stats.contains("memoryUsage")) {
                        status_.memoryUsageMB = stats["memoryUsage"].get<double>();
                    }
                    if (stats.contains("activeFps")) {
                        status_.activeFps = stats["activeFps"].get<double>();
                    }
                    if (stats.contains("outputBytes")) {
                        status_.outputBytes = stats["outputBytes"].get<int64_t>();
                    }
                    if (stats.contains("strain")) {
                        status_.strain = stats["strain"].get<double>();
                    }
                }
            } else if (requestType == "GetVideoInfo") {
                if (j.contains("responseData")) {
                    auto& video = j["responseData"];
                    if (video.contains("encodingLag")) {
                        status_.encodingLagMs = video["encodingLag"].get<double>();
                    }
                    if (video.contains("renderLag")) {
                        status_.renderLagMs = video["renderLag"].get<double>();
                    }
                    // 비디오 설정 파싱
                    if (video.contains("baseWidth")) {
                        status_.baseWidth = video["baseWidth"].get<int>();
                    }
                    if (video.contains("baseHeight")) {
                        status_.baseHeight = video["baseHeight"].get<int>();
                    }
                    if (video.contains("outputWidth")) {
                        status_.outputWidth = video["outputWidth"].get<int>();
                    }
                    if (video.contains("outputHeight")) {
                        status_.outputHeight = video["outputHeight"].get<int>();
                    }
                    if (video.contains("fps")) {
                        status_.fps = video["fps"].get<double>();
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to handle OBS response: {}", e.what());
    }
}

void ObsClient::updateStatus() {
    // 현재 상태 정보 요청
    sendRequest("GetSceneList");
    sendRequest("GetRecordStatus");
    sendRequest("GetStreamStatus");
    getVideoSettings(); // 비디오 설정 요청
    
    // 통계 수집 시작
    if (!stats_running_) {
        stats_running_ = true;
        stats_thread_ = std::thread([this]() {
            while (stats_running_) {
                if (connected_) {
                    requestStats();
                }
                std::this_thread::sleep_for(std::chrono::seconds(5)); // 5초마다 통계 수집
            }
        });
    }
}

void ObsClient::requestStats() {
    // OBS 성능 통계 요청
    sendRequest("GetStats");
    
    // 추가 지표들 요청 (OBS WebSocket API v5 기준)
    sendRequest("GetVideoInfo");
    sendRequest("GetStreamingStatus");
    sendRequest("GetRecordingStatus");
}

// OBS Control methods implementation
bool ObsClient::getSceneList(std::vector<std::string>& scenes) {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "GetSceneList"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        
        // Note: This is a simplified implementation. In a real scenario,
        // you'd need to handle the response asynchronously
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to get scene list: {}", e.what());
        return false;
    }
}

bool ObsClient::setCurrentProgramScene(const std::string& sceneName) {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "SetCurrentProgramScene"},
            {"requestId", requestId_++},
            {"requestData", {
                {"sceneName", sceneName}
            }}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to set current scene: {}", e.what());
        return false;
    }
}

bool ObsClient::startStream() {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "StartStreaming"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start streaming: {}", e.what());
        return false;
    }
}

bool ObsClient::stopStream() {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "StopStreaming"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to stop streaming: {}", e.what());
        return false;
    }
}

bool ObsClient::startRecord() {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "StartRecording"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to start recording: {}", e.what());
        return false;
    }
}

bool ObsClient::stopRecord() {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "StopRecording"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to stop recording: {}", e.what());
        return false;
    }
}

bool ObsClient::getInputList(std::vector<std::string>& inputs) {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "GetInputList"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        
        // Note: This is a simplified implementation. In a real scenario,
        // you'd need to handle the response asynchronously
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to get input list: {}", e.what());
        return false;
    }
}

bool ObsClient::getVideoSettings() {
    if (!connected_) return false;
    
    try {
        json request = {
            {"requestType", "GetVideoInfo"},
            {"requestId", requestId_++}
        };
        
        client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to get video settings: {}", e.what());
        return false;
    }
}

void ObsClient::subscribeToEvents() {
    if (!connected_ || eventsSubscribed_) return;
    
    try {
        // Subscribe to relevant events
        std::vector<std::string> events = {
            "CurrentProgramSceneChanged",
            "StreamStateChanged", 
            "RecordStateChanged",
            "ExitStarted"
        };
        
        for (const auto& event : events) {
            json request = {
                {"requestType", "SubscribeToEvents"},
                {"requestId", requestId_++},
                {"requestData", {
                    {"eventTypes", {event}}
                }}
            };
            
            client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        }
        
        eventsSubscribed_ = true;
        spdlog::info("Subscribed to OBS events");
    } catch (const std::exception& e) {
        spdlog::error("Failed to subscribe to events: {}", e.what());
    }
}

void ObsClient::setEventCallback(std::function<void(const std::string&, const std::string&)> cb) {
    eventCallback_ = std::move(cb);
}
