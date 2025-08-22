#include <doctest/doctest.h>
#include "../src/core/Config.h"
#include <filesystem>

TEST_CASE("Config - JSON Serialization") {
    Config config;
    
    SUBCASE("Default values") {
        CHECK(config.probeHost == "127.0.0.1");
        CHECK(config.probePort == 50051);
        CHECK(config.probeRateHz == 20);
        CHECK(config.obsHost == "localhost");
        CHECK(config.obsPort == 4455);
        CHECK(config.rttThreshold == 100.0);
        CHECK(config.lossThreshold == 5.0);
        CHECK(config.enableDiscord == true);
        CHECK(config.monitoredProcesses.size() == 2);
    }
    
    SUBCASE("Custom values") {
        config.probeHost = "192.168.1.100";
        config.probePort = 8080;
        config.probeRateHz = 30;
        config.obsHost = "192.168.1.200";
        config.obsPort = 4444;
        config.obsPassword = "secret123";
        config.rttThreshold = 150.0;
        config.lossThreshold = 10.0;
        config.enableDiscord = false;
        config.monitoredProcesses = {"test.exe", "app.exe"};
        
        CHECK(config.probeHost == "192.168.1.100");
        CHECK(config.probePort == 8080);
        CHECK(config.probeRateHz == 30);
        CHECK(config.obsHost == "192.168.1.200");
        CHECK(config.obsPort == 4444);
        CHECK(config.obsPassword == "secret123");
        CHECK(config.rttThreshold == 150.0);
        CHECK(config.lossThreshold == 10.0);
        CHECK(config.enableDiscord == false);
        CHECK(config.monitoredProcesses.size() == 2);
        CHECK(config.monitoredProcesses[0] == "test.exe");
        CHECK(config.monitoredProcesses[1] == "app.exe");
    }
}

TEST_CASE("Config - File I/O") {
    Config originalConfig;
    originalConfig.probeHost = "test.host.com";
    originalConfig.probePort = 12345;
    originalConfig.obsHost = "obs.test.com";
    originalConfig.obsPort = 54321;
    originalConfig.obsPassword = "testpassword";
    originalConfig.rttThreshold = 200.0;
    originalConfig.lossThreshold = 15.0;
    originalConfig.enableDiscord = false;
    originalConfig.monitoredProcesses = {"test1.exe", "test2.exe", "test3.exe"};
    
    std::string testFile = "test_config.json";
    
    SUBCASE("Save and load config") {
        // Save config
        bool saveResult = SaveConfig(testFile, originalConfig);
        CHECK(saveResult == true);
        CHECK(std::filesystem::exists(testFile));
        
        // Load config
        Config loadedConfig;
        bool loadResult = LoadConfig(testFile, loadedConfig);
        CHECK(loadResult == true);
        
        // Verify loaded values
        CHECK(loadedConfig.probeHost == originalConfig.probeHost);
        CHECK(loadedConfig.probePort == originalConfig.probePort);
        CHECK(loadedConfig.obsHost == originalConfig.obsHost);
        CHECK(loadedConfig.obsPort == originalConfig.obsPort);
        CHECK(loadedConfig.obsPassword == originalConfig.obsPassword);
        CHECK(loadedConfig.rttThreshold == originalConfig.rttThreshold);
        CHECK(loadedConfig.lossThreshold == originalConfig.lossThreshold);
        CHECK(loadedConfig.enableDiscord == originalConfig.enableDiscord);
        CHECK(loadedConfig.monitoredProcesses.size() == originalConfig.monitoredProcesses.size());
        
        for (size_t i = 0; i < originalConfig.monitoredProcesses.size(); ++i) {
            CHECK(loadedConfig.monitoredProcesses[i] == originalConfig.monitoredProcesses[i]);
        }
        
        // Cleanup
        std::filesystem::remove(testFile);
    }
    
    SUBCASE("Load non-existent file") {
        Config config;
        bool result = LoadConfig("non_existent_file.json", config);
        CHECK(result == false);
    }
}
