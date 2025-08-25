#pragma once
#include <string>

namespace alert {

enum class NotificationChannel {
    DISCORD,
    SLACK,
    EMAIL
};

class Notifier {
public:
    bool sendDiscord(const std::string& content);
    bool sendSlack(const std::string& content);
    bool sendEmail(const std::string& subject, const std::string& content);
    bool isReady() const;
    
    // 채널별 설정
    void setDiscordWebhook(const std::string& webhook_url);
    void setSlackWebhook(const std::string& webhook_url);
    void setEmailConfig(const std::string& smtp_server, int port, 
                       const std::string& username, const std::string& password,
                       const std::string& from_email, const std::string& to_email);
    
    // 채널별 활성화 상태
    void enableChannel(NotificationChannel channel, bool enabled);
    bool isChannelEnabled(NotificationChannel channel) const;
    
    // 통합 알림 전송
    bool sendNotification(const std::string& title, const std::string& message, 
                         const std::string& color = "0x00ff00");
};

// 헬퍼 함수들
void setDiscordWebhook(const std::string& webhook_url);
void setSlackWebhook(const std::string& webhook_url);
bool sendDiscordEmbed(const std::string& title, const std::string& description, 
                     const std::string& color = "0x00ff00");
bool sendSlackMessage(const std::string& message, const std::string& channel = "#general");

} // namespace alert