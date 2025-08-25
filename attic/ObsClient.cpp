#include "ObsClient.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp> // Added for json parsing
#include <optional> // Added for std::optional

using websocketpp::client;
using websocketpp::config::asio_client;
using json = nlohmann::json; // Added for json type

class ObsClient::ObsClientImpl {
public:
    ObsClientImpl() : connected_(false), client_(nullptr), connection_(nullptr) {
        client_ = std::make_unique<client<asio_client>>();
        
        client_->set_access_channels(websocketpp::log::alevel::none);
        client_->set_error_channels(websocketpp::log::elevel::none);
        
        client_->init_asio();
        
        client_->set_open_handler([this](websocketpp::connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(mutex_);
            connected_ = true;
            cv_.notify_one();
        });
        
        client_->set_close_handler([this](websocketpp::connection_hdl hdl) {
            std::lock_guard<std::mutex> lock(mutex_);
            connected_ = false;
        });
        
        client_->set_message_handler([this](websocketpp::connection_hdl hdl, 
                                           client<asio_client>::message_ptr msg) {
            try {
                auto j = json::parse(msg->get_payload());
                if (j.contains("op") && j["op"] == 7) { // RequestResponse
                    std::lock_guard<std::mutex> lock(mutex_);
                    responses_.push(j);
                    cv_.notify_one();
                }
            } catch (const std::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        });
    }
    
    ~ObsClientImpl() {
        disconnect();
    }
    
    bool connect(const std::string& url, const std::string& password) {
        try {
            websocketpp::lib::error_code ec;
            connection_ = client_->get_connection(url, ec);
            
            if (ec) {
                std::cerr << "Connection creation failed: " << ec.message() << std::endl;
                return false;
            }
            
            client_->connect(connection_);
            
            // 별도 스레드에서 WebSocket 루프 실행
            if (!client_thread_.joinable()) {
                client_thread_ = std::thread([this]() {
                    try {
                        client_->run();
                    } catch (const std::exception& e) {
                        std::cerr << "WebSocket run error: " << e.what() << std::endl;
                    }
                });
            }
            
            // 연결 대기
            std::unique_lock<std::mutex> lock(mutex_);
            if (cv_.wait_for(lock, std::chrono::seconds(5), [this] { return connected_; })) {
                // 인증 (패스워드가 있는 경우)
                if (!password.empty()) {
                    json auth_request = {
                        {"op", 1},
                        {"d", {
                            {"rpcVersion", 1},
                            {"authentication", password},
                            {"eventSubscriptions", 0}
                        }}
                    };
                    sendRequest(auth_request);
                }
                return true;
            }
            
            return false;
        } catch (const std::exception& e) {
            std::cerr << "Connection error: " << e.what() << std::endl;
            return false;
        }
    }
    
    void disconnect() {
        if (connection_ && connected_) {
            websocketpp::lib::error_code ec;
            client_->close(connection_, websocketpp::close::status::normal, "Disconnect", ec);
        }
        
        if (client_thread_.joinable()) {
            client_thread_.join();
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = false;
    }
    
    bool isConnected() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return connected_;
    }
    
    bool startStreaming() {
        return sendSimpleRequest("StartStreaming");
    }
    
    bool stopStreaming() {
        return sendSimpleRequest("StopStreaming");
    }
    
    bool startRecording() {
        return sendSimpleRequest("StartRecording");
    }
    
    bool stopRecording() {
        return sendSimpleRequest("StopRecording");
    }
    
    bool setCurrentScene(const std::string& sceneName) {
        json request = {
            {"op", 6},
            {"d", {
                {"requestType", "SetCurrentProgramScene"},
                {"requestId", generateRequestId()},
                {"requestData", {
                    {"sceneName", sceneName}
                }}
            }}
        };
        return sendRequest(request);
    }
    
    std::string getCurrentScene() {
        json request = {
            {"op", 6},
            {"d", {
                {"requestType", "GetCurrentProgramScene"},
                {"requestId", generateRequestId()}
            }}
        };
        
        auto response = sendRequestWithResponse(request);
        if (response && response->contains("responseData") && 
            (*response)["responseData"].contains("currentProgramSceneName")) {
            return (*response)["responseData"]["currentProgramSceneName"];
        }
        return "";
    }
    
    ObsStats getStats() {
        ObsStats stats;
        
        // 스트리밍 상태 확인
        json stream_request = {
            {"op", 6},
            {"d", {
                {"requestType", "GetStreamingStatus"},
                {"requestId", generateRequestId()}
            }}
        };
        
        auto stream_response = sendRequestWithResponse(stream_request);
        if (stream_response && stream_response->contains("responseData")) {
            auto& data = (*stream_response)["responseData"];
            stats.streaming = data.value("outputActive", false);
        }
        
        // 녹화 상태 확인
        json record_request = {
            {"op", 6},
            {"d", {
                {"requestType", "GetRecordingStatus"},
                {"requestId", generateRequestId()}
            }}
        };
        
        auto record_response = sendRequestWithResponse(record_request);
        if (record_response && record_response->contains("responseData")) {
            auto& data = (*record_response)["responseData"];
            stats.recording = data.value("outputActive", false);
        }
        
        // 현재 씬 이름
        stats.current_scene = getCurrentScene();
        
        // 성능 통계 (실제로는 OBS에서 제공하는 API 사용)
        // 여기서는 기본값 사용
        stats.dropped_frames = 0.0;
        stats.encoding_lag_ms = 0.0;
        stats.render_lag_ms = 0.0;
        
        return stats;
    }
    
private:
    std::unique_ptr<client<asio_client>> client_;
    websocketpp::connection_hdl connection_;
    std::thread client_thread_;
    std::atomic<bool> connected_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<json> responses_;
    
    void sendRequest(const json& request) {
        if (!connected_) return;
        
        try {
            client_->send(connection_, request.dump(), websocketpp::frame::opcode::text);
        } catch (const std::exception& e) {
            std::cerr << "Send error: " << e.what() << std::endl;
        }
    }
    
    bool sendSimpleRequest(const std::string& requestType) {
        json request = {
            {"op", 6},
            {"d", {
                {"requestType", requestType},
                {"requestId", generateRequestId()}
            }}
        };
        return sendRequest(request);
    }
    
    std::optional<json> sendRequestWithResponse(const json& request) {
        if (!connected_) return std::nullopt;
        
        std::string requestId = request["d"]["requestId"];
        
        // 요청 전송
        sendRequest(request);
        
        // 응답 대기
        std::unique_lock<std::mutex> lock(mutex_);
        if (cv_.wait_for(lock, std::chrono::seconds(3), [this, &requestId] {
            return !responses_.empty() && 
                   responses_.front().contains("d") && 
                   responses_.front()["d"].contains("requestId") &&
                   responses_.front()["d"]["requestId"] == requestId;
        })) {
            auto response = responses_.front();
            responses_.pop();
            return response;
        }
        
        return std::nullopt;
    }
    
    std::string generateRequestId() {
        static int counter = 0;
        return "req_" + std::to_string(++counter);
    }
};

// ObsClient 구현
ObsClient::ObsClient() : impl_(std::make_unique<ObsClientImpl>()) {}

ObsClient::~ObsClient() = default;

bool ObsClient::connect(const std::string& url, const std::string& password) {
    return impl_->connect(url, password);
}

void ObsClient::disconnect() {
    impl_->disconnect();
}

bool ObsClient::isConnected() const {
    return impl_->isConnected();
}

bool ObsClient::startStreaming() {
    return impl_->startStreaming();
}

bool ObsClient::stopStreaming() {
    return impl_->stopStreaming();
}

bool ObsClient::startRecording() {
    return impl_->startRecording();
}

bool ObsClient::stopRecording() {
    return impl_->stopRecording();
}

bool ObsClient::setCurrentScene(const std::string& sceneName) {
    return impl_->setCurrentScene(sceneName);
}

std::string ObsClient::getCurrentScene() {
    return impl_->getCurrentScene();
}

ObsStats ObsClient::getStats() {
    return impl_->getStats();
}

