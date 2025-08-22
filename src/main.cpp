#include "app/App.h"
#include "net/Probe.h"
#include "core/Config.h"
#include <iostream>
#include <string>

void printUsage() {
    std::cout << "LiveOps Sentinel - UE/OBS Live Streaming Quality Monitor\n";
    std::cout << "Usage: liveops_sentinel [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --webhook=\"URL\"     Set Discord webhook URL and save to config\n";
    std::cout << "  --reset-webhook     Clear webhook configuration and exit\n";
    std::cout << "  --help              Show this help message\n";
}

int main(int argc, char* argv[]) {
    // CLI 인자 처리
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        else if (arg.substr(0, 10) == "--webhook=") {
            std::string webhookUrl = arg.substr(10);
            
            // URL 유효성 검사 (간단한 검사)
            if (webhookUrl.find("https://discord.com/api/webhooks/") == 0 || 
                webhookUrl.find("https://discordapp.com/api/webhooks/") == 0) {
                
                Config config;
                LoadUserConfig(config); // 기존 설정 로드
                config.discordWebhook = webhookUrl;
                config.webhookConfigured = true;
                
                if (SaveUserConfig(config)) {
                    std::cout << "Webhook URL saved successfully: " << MaskWebhook(webhookUrl) << std::endl;
                } else {
                    std::cerr << "Failed to save webhook URL" << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Invalid Discord webhook URL format" << std::endl;
                std::cerr << "Expected format: https://discord.com/api/webhooks/ID/TOKEN" << std::endl;
                return 1;
            }
        }
        else if (arg == "--reset-webhook") {
            Config config;
            LoadUserConfig(config);
            config.discordWebhook.clear();
            config.webhookConfigured = false;
            
            if (SaveUserConfig(config)) {
                std::cout << "Webhook configuration cleared successfully" << std::endl;
            } else {
                std::cerr << "Failed to clear webhook configuration" << std::endl;
                return 1;
            }
            return 0; // 설정만 변경하고 종료
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }

    // 로컬 테스트 편의: 50051 UDP 에코 서버 자동 시작
    Probe::startLocalEcho(50051);

    App app;
    if (!app.init()) return 1;
    app.run();

    Probe::stopLocalEcho();
    return 0;
}
