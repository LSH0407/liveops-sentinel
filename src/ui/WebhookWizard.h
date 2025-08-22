#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

namespace ui {

class WebhookWizard {
public:
    struct TestResult {
        bool success{false};
        std::string message;
        int statusCode{0};
    };

    WebhookWizard();
    ~WebhookWizard();

    // Show the wizard modal (non-closable until valid webhook is saved)
    void show();
    
    // Check if wizard should be shown (webhook not configured)
    static bool shouldShowWizard();
    
    // Set callback for when webhook is successfully saved
    void setOnWebhookSaved(std::function<void(const std::string&)> callback);

private:
    void drawModal();
    void drawInputField();
    void drawButtons();
    void testWebhook();
    void saveWebhook();
    bool validateWebhookUrl(const std::string& url) const;
    std::string sanitizeUrl(const std::string& url) const;
    
    // Test webhook in background thread
    void testWebhookAsync();
    TestResult performWebhookTest(const std::string& url) const;

    std::string webhookUrl_;
    std::string maskedUrl_;
    bool showPassword_{false};
    bool isVisible_{false};
    bool isTesting_{false};
    bool isValidUrl_{false};
    
    TestResult lastTestResult_;
    std::function<void(const std::string&)> onWebhookSaved_;
    
    // Background test thread
    std::unique_ptr<std::thread> testThread_;
    std::atomic<bool> testThreadRunning_{false};
    mutable std::mutex testMutex_;
};

} // namespace ui
