#include "Notifier.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <map>

using json = nlohmann::json;

namespace alert {

class NotifierImpl {
public:
    NotifierImpl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        // 기본 채널 활성화 상태
        enabled_channels_[NotificationChannel::DISCORD] = false;
        enabled_channels_[NotificationChannel::SLACK] = false;
        enabled_channels_[NotificationChannel::EMAIL] = false;
    }
    
    ~NotifierImpl() {
        curl_global_cleanup();
    }
    
    // Discord 설정
    void setDiscordWebhook(const std::string& url) {
        discord_webhook_url_ = url;
        enabled_channels_[NotificationChannel::DISCORD] = !url.empty();
    }
    
    // Slack 설정
    void setSlackWebhook(const std::string& url) {
        slack_webhook_url_ = url;
        enabled_channels_[NotificationChannel::SLACK] = !url.empty();
    }
    
    // Email 설정
    void setEmailConfig(const std::string& smtp_server, int port,
                       const std::string& username, const std::string& password,
                       const std::string& from_email, const std::string& to_email) {
        email_config_ = {
            {"smtp_server", smtp_server},
            {"port", port},
            {"username", username},
            {"password", password},
            {"from_email", from_email},
            {"to_email", to_email}
        };
        enabled_channels_[NotificationChannel::EMAIL] = !smtp_server.empty();
    }
    
    // 채널 활성화 상태 관리
    void enableChannel(NotificationChannel channel, bool enabled) {
        enabled_channels_[channel] = enabled;
    }
    
    bool isChannelEnabled(NotificationChannel channel) const {
        auto it = enabled_channels_.find(channel);
        return it != enabled_channels_.end() && it->second;
    }
    
    // Discord 메시지 전송
    bool sendDiscord(const std::string& content) {
        if (!isChannelEnabled(NotificationChannel::DISCORD) || discord_webhook_url_.empty()) {
            return false;
        }
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        json payload = {
            {"content", content},
            {"username", "LiveOps Sentinel"}
        };
        
        std::string json_str = payload.dump();
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, discord_webhook_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK && http_code == 204);
    }
    
    // Discord 임베드 전송
    bool sendDiscordEmbed(const std::string& title, const std::string& description, 
                         const std::string& color = "0x00ff00") {
        if (!isChannelEnabled(NotificationChannel::DISCORD) || discord_webhook_url_.empty()) {
            return false;
        }
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        json embed = {
            {"title", title},
            {"description", description},
            {"color", std::stoi(color.substr(2), nullptr, 16)},
            {"timestamp", getCurrentTimestamp()}
        };
        
        json payload = {
            {"embeds", {embed}},
            {"username", "LiveOps Sentinel"}
        };
        
        std::string json_str = payload.dump();
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, discord_webhook_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK && http_code == 204);
    }
    
    // Slack 메시지 전송
    bool sendSlack(const std::string& content) {
        if (!isChannelEnabled(NotificationChannel::SLACK) || slack_webhook_url_.empty()) {
            return false;
        }
        
        CURL* curl = curl_easy_init();
        if (!curl) return false;
        
        json payload = {
            {"text", content},
            {"username", "LiveOps Sentinel"}
        };
        
        std::string json_str = payload.dump();
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, slack_webhook_url_.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        return (res == CURLE_OK && http_code == 200);
    }
    
    // Email 전송 (간단한 구현)
    bool sendEmail(const std::string& subject, const std::string& content) {
        if (!isChannelEnabled(NotificationChannel::EMAIL) || email_config_.empty()) {
            return false;
        }
        
        // 실제 구현에서는 SMTP 라이브러리 사용 필요
        // 여기서는 로그만 출력
        std::cout << "Email would be sent:" << std::endl;
        std::cout << "To: " << email_config_["to_email"] << std::endl;
        std::cout << "Subject: " << subject << std::endl;
        std::cout << "Content: " << content << std::endl;
        
        return true;
    }
    
    // 통합 알림 전송
    bool sendNotification(const std::string& title, const std::string& message, 
                         const std::string& color = "0x00ff00") {
        bool success = true;
        
        // Discord로 전송
        if (isChannelEnabled(NotificationChannel::DISCORD)) {
            success &= sendDiscordEmbed(title, message, color);
        }
        
        // Slack으로 전송
        if (isChannelEnabled(NotificationChannel::SLACK)) {
            success &= sendSlack(title + ": " + message);
        }
        
        // Email로 전송
        if (isChannelEnabled(NotificationChannel::EMAIL)) {
            success &= sendEmail(title, message);
        }
        
        return success;
    }
    
    bool isReady() const {
        return !discord_webhook_url_.empty() || !slack_webhook_url_.empty() || !email_config_.empty();
    }
    
private:
    std::string discord_webhook_url_;
    std::string slack_webhook_url_;
    json email_config_;
    std::map<NotificationChannel, bool> enabled_channels_;
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return ss.str();
    }
};

// 전역 Notifier 인스턴스
static std::unique_ptr<NotifierImpl> g_notifier = std::make_unique<NotifierImpl>();

// Notifier 클래스 구현
bool Notifier::sendDiscord(const std::string& content) {
    return g_notifier->sendDiscord(content);
}

bool Notifier::sendSlack(const std::string& content) {
    return g_notifier->sendSlack(content);
}

bool Notifier::sendEmail(const std::string& subject, const std::string& content) {
    return g_notifier->sendEmail(subject, content);
}

bool Notifier::isReady() const {
    return g_notifier->isReady();
}

void Notifier::setDiscordWebhook(const std::string& webhook_url) {
    g_notifier->setDiscordWebhook(webhook_url);
}

void Notifier::setSlackWebhook(const std::string& webhook_url) {
    g_notifier->setSlackWebhook(webhook_url);
}

void Notifier::setEmailConfig(const std::string& smtp_server, int port,
                             const std::string& username, const std::string& password,
                             const std::string& from_email, const std::string& to_email) {
    g_notifier->setEmailConfig(smtp_server, port, username, password, from_email, to_email);
}

void Notifier::enableChannel(NotificationChannel channel, bool enabled) {
    g_notifier->enableChannel(channel, enabled);
}

bool Notifier::isChannelEnabled(NotificationChannel channel) const {
    return g_notifier->isChannelEnabled(channel);
}

bool Notifier::sendNotification(const std::string& title, const std::string& message, 
                               const std::string& color) {
    return g_notifier->sendNotification(title, message, color);
}

// 헬퍼 함수들
void setDiscordWebhook(const std::string& webhook_url) {
    g_notifier->setDiscordWebhook(webhook_url);
}

void setSlackWebhook(const std::string& webhook_url) {
    g_notifier->setSlackWebhook(webhook_url);
}

bool sendDiscordEmbed(const std::string& title, const std::string& description, 
                     const std::string& color) {
    return g_notifier->sendDiscordEmbed(title, description, color);
}

bool sendSlackMessage(const std::string& message, const std::string& channel) {
    return g_notifier->sendSlack(message);
}

} // namespace alert
