#pragma once
#include <string>
#include <functional>
#include <memory>
#include <asio.hpp>
#include <chrono>
#include <unordered_map>

enum class AlertLevel {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class AlertType {
    RTT,
    LOSS,
    ENCODE_LAG,
    RENDER_LAG,
    DROPPED_FRAMES,
    GENERAL
};

struct AlertConfig {
    double rttThreshold{100.0};  // ms
    double lossThreshold{5.0};   // %
    bool enableDiscord{true};
    std::string discordWebhook;
    std::string discordUsername{"LiveOps Sentinel"};
    int cooldownSec{60};
    bool webhookConfigured{false};
};

struct AggregatedAlert {
    AlertLevel level;
    std::string title;
    std::string message;
    std::chrono::steady_clock::time_point firstOccurrence;
    std::chrono::steady_clock::time_point lastOccurrence;
    int occurrenceCount{0};
    double maxValue{0.0};
    double avgValue{0.0};
    double totalValue{0.0};
};

class Notifier {
public:
    Notifier();
    ~Notifier();
    
    void setConfig(const AlertConfig& config);
    void sendAlert(AlertLevel level, const std::string& title, const std::string& message);
    void sendAlertWithMetrics(AlertLevel level, const std::string& title, const std::string& message, 
                             const std::string& metricsJson);
    void sendAlertWithCooldown(AlertType type, AlertLevel level, const std::string& title, 
                              const std::string& message, double value = 0.0);
    void sendDiscordAlert(const std::string& content, const std::string& username = "");
    bool isWebhookConfigured() const { return config_.webhookConfigured && !config_.discordWebhook.empty(); }
    
private:
    bool sendDiscordWebhook(const std::string& webhook, const std::string& content, const std::string& username);
    std::string createAggregatedMessage(const AggregatedAlert& alert);
    void flushAggregatedAlerts();
    
    AlertConfig config_;
    std::unique_ptr<asio::io_context> io_context_;
    std::unique_ptr<asio::ip::tcp::resolver> resolver_;
    
    // 쿨다운 추적
    std::unordered_map<AlertType, AggregatedAlert> aggregatedAlerts_;
    std::chrono::steady_clock::time_point lastFlush_;
};
