#include <doctest/doctest.h>
#include "../src/core/ReportWriter.h"
#include <filesystem>

TEST_SUITE("ReportWriter") {
    TEST_CASE("Basic snapshot addition") {
        core::ReportConfig config;
        config.enable = true;
        config.flushIntervalSec = 1;
        config.dir = "test_reports";
        
        core::ReportWriter writer(config);
        
        // Add some snapshots
        writer.addSnapshot(10.5, 2.1, 0.05, 15.2, 45.0, 60.0, 2048.0);
        writer.addSnapshot(12.3, 1.8, 0.03, 14.8, 48.0, 65.0, 2100.0);
        
        // Force flush
        writer.flushNow();
        
        // Check if files were created
        std::filesystem::path dir(config.dir);
        bool csvFound = false, jsonFound = false;
        
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".csv") csvFound = true;
            if (entry.path().extension() == ".json") jsonFound = true;
        }
        
        CHECK(csvFound);
        CHECK(jsonFound);
        
        // Clean up
        std::filesystem::remove_all(dir);
    }
    
    TEST_CASE("CSV header consistency") {
        core::ReportConfig config;
        config.enable = true;
        config.dir = "test_reports";
        
        core::ReportWriter writer(config);
        
        // Add a snapshot and flush
        writer.addSnapshot(10.0, 1.0, 0.02, 16.0, 50.0, 70.0, 2000.0);
        writer.flushNow();
        
        // Check CSV file content
        std::filesystem::path dir(config.dir);
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".csv") {
                std::ifstream file(entry.path());
                std::string line;
                std::getline(file, line);
                
                // Check header
                std::string expectedHeader = "ts,rtt_ms,loss_pct,obs_dropped_ratio,avg_render_ms,cpu_pct,gpu_pct,mem_mb";
                CHECK(line == expectedHeader);
                break;
            }
        }
        
        // Clean up
        std::filesystem::remove_all(dir);
    }
    
    TEST_CASE("JSON structure consistency") {
        core::ReportConfig config;
        config.enable = true;
        config.dir = "test_reports";
        
        core::ReportWriter writer(config);
        
        // Add a snapshot and flush
        writer.addSnapshot(10.0, 1.0, 0.02, 16.0, 50.0, 70.0, 2000.0);
        writer.flushNow();
        
        // Check JSON file content
        std::filesystem::path dir(config.dir);
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.path().extension() == ".json") {
                std::ifstream file(entry.path());
                nlohmann::json j;
                file >> j;
                
                // Check structure
                CHECK(j.contains("metadata"));
                CHECK(j.contains("snapshots"));
                CHECK(j["snapshots"].is_array());
                CHECK(j["snapshots"].size() == 1);
                
                auto& snapshot = j["snapshots"][0];
                CHECK(snapshot.contains("rtt_ms"));
                CHECK(snapshot.contains("loss_pct"));
                CHECK(snapshot.contains("obs_dropped_ratio"));
                CHECK(snapshot.contains("avg_render_ms"));
                CHECK(snapshot.contains("cpu_pct"));
                CHECK(snapshot.contains("gpu_pct"));
                CHECK(snapshot.contains("mem_mb"));
                
                break;
            }
        }
        
        // Clean up
        std::filesystem::remove_all(dir);
    }
    
    TEST_CASE("Disabled writer behavior") {
        core::ReportConfig config;
        config.enable = false;
        config.dir = "test_reports";
        
        core::ReportWriter writer(config);
        
        // Add snapshots (should be ignored)
        writer.addSnapshot(10.0, 1.0, 0.02, 16.0, 50.0, 70.0, 2000.0);
        writer.flushNow();
        
        // Check that no files were created
        std::filesystem::path dir(config.dir);
        CHECK(!std::filesystem::exists(dir));
    }
}
