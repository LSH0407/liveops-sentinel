#include "WebhookWizard.h"
#include "../core/Config.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace ui {

WebhookWizard::WebhookWizard() {
    // Initialize Winsock on Windows
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

WebhookWizard::~WebhookWizard() {
    if (testThread_ && testThread_->joinable()) {
        testThreadRunning_ = false;
        testThread_->join();
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
}

void WebhookWizard::show() {
    isVisible_ = true;
    drawModal();
}

bool WebhookWizard::shouldShowWizard() {
    Config config;
    if (!LoadUserConfig(config)) {
        return true; // Show wizard if no config file exists
    }
    return !config.webhookConfigured || config.discordWebhook.empty();
}

void WebhookWizard::setOnWebhookSaved(std::function<void(const std::string&)> callback) {
    onWebhookSaved_ = callback;
}

void WebhookWizard::drawModal() {
    // Console mode - GUI disabled
    std::cout << "Webhook Wizard Modal: Console mode" << std::endl;
}

void WebhookWizard::drawInputField() {
    // Console mode - GUI disabled
    std::cout << "Webhook Input Field: Console mode" << std::endl;
}

void WebhookWizard::drawButtons() {
    // Console mode - GUI disabled
    std::cout << "Webhook Buttons: Console mode" << std::endl;
}

void WebhookWizard::testWebhook() {
    if (webhookUrl_.empty() || !isValidUrl_) {
        lastTestResult_ = {false, "유효하지 않은 웹훅 URL입니다.", 0};
        return;
    }
    
    if (testThreadRunning_) return; // Prevent multiple concurrent tests
    
    isTesting_ = true;
    lastTestResult_ = {false, "테스트를 시작합니다...", 0};
    
    // Start background test thread
    testThreadRunning_ = true;
    testThread_ = std::make_unique<std::thread>([this]() {
        testWebhookAsync();
    });
}

void WebhookWizard::testWebhookAsync() {
    auto result = performWebhookTest(webhookUrl_);
    
    std::lock_guard<std::mutex> lock(testMutex_);
    lastTestResult_ = result;
    isTesting_ = false;
    testThreadRunning_ = false;
}

WebhookWizard::TestResult WebhookWizard::performWebhookTest(const std::string& url) const {
    // Parse URL
    std::regex urlRegex(R"(https://(discord\.com|discordapp\.com)/api/webhooks/(\d+)/([^/]+))");
    std::smatch match;
    
    if (!std::regex_match(url, match, urlRegex)) {
        return {false, "잘못된 Discord 웹훅 URL 형식입니다.", 0};
    }
    
    std::string host = match[1].str();
    std::string webhookId = match[2].str();
    std::string webhookToken = match[3].str();
    
    // Create HTTP request
    std::string request = 
        "POST /api/webhooks/" + webhookId + "/" + webhookToken + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 67\r\n"
        "\r\n"
        "{\"content\":\"LiveOps Sentinel test: ✅ Webhook OK\"}";
    
    // Create socket
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return {false, "소켓 생성 실패", 0};
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return {false, "소켓 생성 실패", 0};
    }
#endif
    
    // Set timeout
#ifdef _WIN32
    DWORD timeout = 5000; // 5 seconds
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
    
    // Connect to Discord
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(443); // HTTPS port
    
    struct hostent* hostEntry = gethostbyname(host.c_str());
    if (!hostEntry) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return {false, "호스트 이름을 찾을 수 없습니다: " + host, 0};
    }
    
    server.sin_addr = *((struct in_addr*)hostEntry->h_addr_list[0]);
    
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return {false, "Discord 서버에 연결할 수 없습니다", 0};
    }
    
    // Send request
    if (send(sock, request.c_str(), request.length(), 0) < 0) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return {false, "요청 전송 실패", 0};
    }
    
    // Receive response
    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    
    if (bytesReceived <= 0) {
        return {false, "응답을 받을 수 없습니다", 0};
    }
    
    buffer[bytesReceived] = '\0';
    std::string response(buffer);
    
    // Parse HTTP status line
    std::istringstream responseStream(response);
    std::string statusLine;
    std::getline(responseStream, statusLine);
    
    // Extract status code
    std::regex statusRegex(R"(HTTP/\d\.\d\s+(\d+))");
    std::smatch statusMatch;
    
    if (std::regex_search(statusLine, statusMatch, statusRegex)) {
        int statusCode = std::stoi(statusMatch[1].str());
        
        if (statusCode == 204 || statusCode == 200) {
            return {true, "웹훅 테스트 성공! Discord로 메시지가 전송되었습니다.", statusCode};
        } else {
            return {false, "웹훅 테스트 실패: HTTP " + std::to_string(statusCode), statusCode};
        }
    }
    
    return {false, "응답을 파싱할 수 없습니다", 0};
}

void WebhookWizard::saveWebhook() {
    if (!isValidUrl_) {
        lastTestResult_ = {false, "유효하지 않은 웹훅 URL입니다.", 0};
        return;
    }
    
    Config config;
    LoadUserConfig(config); // Load existing config or create new one
    
    config.discordWebhook = webhookUrl_;
    config.webhookConfigured = true;
    
    if (SaveUserConfig(config)) {
        if (onWebhookSaved_) {
            onWebhookSaved_(webhookUrl_);
        }
        isVisible_ = false;
        lastTestResult_ = {true, "웹훅이 성공적으로 저장되었습니다!", 0};
    } else {
        lastTestResult_ = {false, "웹훅 저장에 실패했습니다.", 0};
    }
}

bool WebhookWizard::validateWebhookUrl(const std::string& url) const {
    if (url.empty()) return false;
    
    // Check for Discord webhook URL pattern
    std::regex webhookRegex(R"(^https://(discord\.com|discordapp\.com)/api/webhooks/\d+/[^/]+$)");
    return std::regex_match(url, webhookRegex);
}

std::string WebhookWizard::sanitizeUrl(const std::string& url) const {
    std::string sanitized = url;
    
    // Remove whitespace and newlines
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\n'), sanitized.end());
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\r'), sanitized.end());
    
    // Trim whitespace
    sanitized.erase(0, sanitized.find_first_not_of(" \t"));
    sanitized.erase(sanitized.find_last_not_of(" \t") + 1);
    
    return sanitized;
}

} // namespace ui
