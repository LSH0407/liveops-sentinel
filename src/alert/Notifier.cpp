#include "Notifier.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <asio/connect.hpp>
#include <asio/write.hpp>
#include <asio/read.hpp>
#include <sstream>

using json = nlohmann::json;

Notifier::Notifier() {
    io_context_ = std::make_unique<asio::io_context>();
    resolver_ = std::make_unique<asio::ip::tcp::resolver>(*io_context_);
}

Notifier::~Notifier() = default;

void Notifier::setConfig(const AlertConfig& config) {
    config_ = config;
}

void Notifier::sendAlert(AlertLevel level, const std::string& title, const std::string& message) {
    std::string levelStr;
    switch (level) {
        case AlertLevel::INFO: levelStr = "ℹ️ INFO"; break;
        case AlertLevel::WARNING: levelStr = "⚠️ WARNING"; break;
        case AlertLevel::ERROR: levelStr = "❌ ERROR"; break;
        case AlertLevel::CRITICAL: levelStr = "🚨 CRITICAL"; break;
    }
    
    std::string content = levelStr + " **" + title + "**\n" + message;
    
    if (config_.enableDiscord && isWebhookConfigured()) {
        sendDiscordAlert(content);
    } else {
        spdlog::warn("Discord webhook not configured - alert not sent: {} - {}", title, message);
    }
    
    spdlog::info("Alert sent: {} - {}", title, message);
}

void Notifier::sendAlertWithMetrics(AlertLevel level, const std::string& title, const std::string& message, 
                                   const std::string& metricsJson) {
    std::string levelStr;
    switch (level) {
        case AlertLevel::INFO: levelStr = "ℹ️ INFO"; break;
        case AlertLevel::WARNING: levelStr = "⚠️ WARNING"; break;
        case AlertLevel::ERROR: levelStr = "❌ ERROR"; break;
        case AlertLevel::CRITICAL: levelStr = "🚨 CRITICAL"; break;
    }
    
    std::string content = levelStr + " **" + title + "**\n" + message;
    
    // Add metrics as a code block
    if (!metricsJson.empty()) {
        content += "\n\n**Recent Metrics (10s snapshot):**\n```json\n" + metricsJson + "\n```";
    }
    
    if (config_.enableDiscord && isWebhookConfigured()) {
        sendDiscordAlert(content);
    } else {
        spdlog::warn("Discord webhook not configured - alert with metrics not sent: {} - {}", title, message);
    }
    
    spdlog::info("Alert with metrics sent: {} - {}", title, message);
}

void Notifier::sendAlertWithCooldown(AlertType type, AlertLevel level, const std::string& title, 
                                    const std::string& message, double value) {
    auto now = std::chrono::steady_clock::now();
    
    // 기존 알림이 있는지 확인
    auto it = aggregatedAlerts_.find(type);
    if (it != aggregatedAlerts_.end()) {
        auto& alert = it->second;
        auto timeSinceLast = std::chrono::duration_cast<std::chrono::seconds>(now - alert.lastOccurrence);
        
        // 쿨다운 기간 내라면 집계
        if (timeSinceLast.count() < config_.cooldownSec) {
            alert.occurrenceCount++;
            alert.lastOccurrence = now;
            alert.totalValue += value;
            alert.avgValue = alert.totalValue / alert.occurrenceCount;
            if (value > alert.maxValue) {
                alert.maxValue = value;
            }
            return; // 쿨다운 중이므로 알림 전송하지 않음
        }
    }
    
    // 새로운 알림 생성 또는 쿨다운 만료
    AggregatedAlert newAlert;
    newAlert.level = level;
    newAlert.title = title;
    newAlert.message = message;
    newAlert.firstOccurrence = now;
    newAlert.lastOccurrence = now;
    newAlert.occurrenceCount = 1;
    newAlert.maxValue = value;
    newAlert.avgValue = value;
    newAlert.totalValue = value;
    
    aggregatedAlerts_[type] = newAlert;
    
    // 즉시 알림 전송
    sendAlert(level, title, message);
}

void Notifier::sendDiscordAlert(const std::string& content, const std::string& username) {
    if (!isWebhookConfigured()) {
        spdlog::warn("Discord webhook not configured");
        return;
    }
    
    std::string user = username.empty() ? config_.discordUsername : username;
    sendDiscordWebhook(config_.discordWebhook, content, user);
}

bool Notifier::sendDiscordWebhook(const std::string& webhook, const std::string& content, const std::string& username) {
    try {
        // Discord 웹후크 URL에서 호스트와 포트 추출
        std::string url = webhook;
        if (url.find("https://") == 0) {
            url = url.substr(8);
        }
        
        size_t slashPos = url.find('/');
        if (slashPos == std::string::npos) {
            spdlog::error("Invalid Discord webhook URL");
            return false;
        }
        
        std::string host = url.substr(0, slashPos);
        std::string path = url.substr(slashPos);
        
        // JSON 페이로드 생성
        json payload = {
            {"content", content},
            {"username", username}
        };
        
        std::string jsonStr = payload.dump();
        
        // HTTP 요청 생성
        std::ostringstream request;
        request << "POST " << path << " HTTP/1.1\r\n";
        request << "Host: " << host << "\r\n";
        request << "Content-Type: application/json\r\n";
        request << "Content-Length: " << jsonStr.length() << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";
        request << jsonStr;
        
        std::string requestStr = request.str();
        
        // TCP 연결 및 요청 전송
        asio::ip::tcp::socket socket(*io_context_);
        asio::ip::tcp::resolver::results_type endpoints = resolver_->resolve(host, "443");
        
        asio::connect(socket, endpoints);
        asio::write(socket, asio::buffer(requestStr));
        
        // 응답 읽기 (간단한 구현)
        std::vector<char> response(1024);
        asio::error_code ec;
        size_t len = socket.read_some(asio::buffer(response), ec);
        
        if (ec) {
            spdlog::error("Failed to send Discord webhook: {}", ec.message());
            return false;
        }
        
        spdlog::info("Discord webhook sent successfully");
        return true;
        
    } catch (const std::exception& e) {
        spdlog::error("Exception sending Discord webhook: {}", e.what());
        return false;
    }
}

std::string Notifier::createAggregatedMessage(const AggregatedAlert& alert) {
    std::stringstream ss;
    ss << alert.title << "\n";
    ss << "발생 횟수: " << alert.occurrenceCount << "회\n";
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        alert.lastOccurrence - alert.firstOccurrence);
    ss << "지속 시간: " << duration.count() << "초\n";
    
    if (alert.maxValue > 0.0) {
        ss << "최대값: " << std::fixed << std::setprecision(2) << alert.maxValue << "\n";
        ss << "평균값: " << std::fixed << std::setprecision(2) << alert.avgValue << "\n";
    }
    
    return ss.str();
}

void Notifier::flushAggregatedAlerts() {
    auto now = std::chrono::steady_clock::now();
    
    // 쿨다운이 만료된 알림들을 정리
    for (auto it = aggregatedAlerts_.begin(); it != aggregatedAlerts_.end();) {
        auto timeSinceLast = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.lastOccurrence);
        
        if (timeSinceLast.count() >= config_.cooldownSec * 2) { // 쿨다운의 2배 시간 후 정리
            it = aggregatedAlerts_.erase(it);
        } else {
            ++it;
        }
    }
}
