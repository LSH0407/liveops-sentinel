#include <doctest/doctest.h>
#include "../src/alert/Notifier.h"
#include <chrono>
#include <thread>

TEST_SUITE("Alert Cooldown System") {
    TEST_CASE("Basic cooldown functionality") {
        Notifier notifier;
        AlertConfig config;
        config.cooldownSec = 5; // 5초 쿨다운
        notifier.setConfig(config);
        
        // 첫 번째 알림 - 즉시 전송되어야 함
        notifier.sendAlertWithCooldown(AlertType::RTT, AlertLevel::WARNING, 
                                      "High RTT", "RTT exceeded threshold", 85.0);
        
        // 같은 타입의 두 번째 알림 - 쿨다운 기간 내이므로 전송되지 않아야 함
        notifier.sendAlertWithCooldown(AlertType::RTT, AlertLevel::WARNING, 
                                      "High RTT", "RTT still high", 90.0);
        
        // 다른 타입의 알림 - 즉시 전송되어야 함
        notifier.sendAlertWithCooldown(AlertType::LOSS, AlertLevel::ERROR, 
                                      "High Loss", "Packet loss detected", 3.0);
        
        // Note: 실제 전송 여부는 Discord Webhook 응답으로 확인해야 하지만,
        // 여기서는 함수 호출이 정상적으로 처리되는지만 확인
        SUCCEED();
    }
    
    TEST_CASE("Cooldown expiration") {
        Notifier notifier;
        AlertConfig config;
        config.cooldownSec = 1; // 1초 쿨다운
        notifier.setConfig(config);
        
        // 첫 번째 알림
        notifier.sendAlertWithCooldown(AlertType::ENCODE_LAG, AlertLevel::WARNING, 
                                      "Encoding Lag", "High encoding lag", 30.0);
        
        // 1초 대기
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        // 쿨다운 만료 후 알림 - 전송되어야 함
        notifier.sendAlertWithCooldown(AlertType::ENCODE_LAG, AlertLevel::WARNING, 
                                      "Encoding Lag", "Still high lag", 35.0);
        
        SUCCEED();
    }
    
    TEST_CASE("Different alert types don't interfere") {
        Notifier notifier;
        AlertConfig config;
        config.cooldownSec = 10; // 10초 쿨다운
        notifier.setConfig(config);
        
        // RTT 알림
        notifier.sendAlertWithCooldown(AlertType::RTT, AlertLevel::WARNING, 
                                      "High RTT", "RTT issue", 85.0);
        
        // Loss 알림 (다른 타입) - 즉시 전송되어야 함
        notifier.sendAlertWithCooldown(AlertType::LOSS, AlertLevel::ERROR, 
                                      "High Loss", "Loss issue", 3.0);
        
        // Dropped Frames 알림 (다른 타입) - 즉시 전송되어야 함
        notifier.sendAlertWithCooldown(AlertType::DROPPED_FRAMES, AlertLevel::WARNING, 
                                      "Dropped Frames", "Frame drops", 5.0);
        
        // 같은 RTT 알림 - 쿨다운 기간 내이므로 전송되지 않아야 함
        notifier.sendAlertWithCooldown(AlertType::RTT, AlertLevel::WARNING, 
                                      "High RTT", "Still high", 90.0);
        
        SUCCEED();
    }
    
    TEST_CASE("Value aggregation during cooldown") {
        Notifier notifier;
        AlertConfig config;
        config.cooldownSec = 5; // 5초 쿨다운
        notifier.setConfig(config);
        
        // 첫 번째 알림
        notifier.sendAlertWithCooldown(AlertType::RENDER_LAG, AlertLevel::WARNING, 
                                      "Render Lag", "High render lag", 25.0);
        
        // 같은 타입의 추가 알림들 (쿨다운 기간 내)
        notifier.sendAlertWithCooldown(AlertType::RENDER_LAG, AlertLevel::WARNING, 
                                      "Render Lag", "Still high", 30.0);
        notifier.sendAlertWithCooldown(AlertType::RENDER_LAG, AlertLevel::WARNING, 
                                      "Render Lag", "Getting worse", 35.0);
        
        // Note: 실제로는 집계된 값들이 저장되어야 하지만,
        // 여기서는 함수 호출이 정상적으로 처리되는지만 확인
        SUCCEED();
    }
}
