#include <doctest/doctest.h>
#include "../src/obs/EventLog.h"
#include <nlohmann/json.hpp>

TEST_SUITE("EventLog") {
    TEST_CASE("Basic push and get operations") {
        obs::EventLog log(10);
        
        // Add some events
        log.push(obs::Event("test1", nlohmann::json{{"value", 1}}));
        log.push(obs::Event("test2", nlohmann::json{{"value", 2}}));
        
        auto events = log.getRecentEvents(5);
        CHECK(events.size() == 2);
        CHECK(events[0].type == "test1");
        CHECK(events[1].type == "test2");
    }
    
    TEST_CASE("Circular buffer behavior") {
        obs::EventLog log(3);
        
        // Add more events than buffer size
        for (int i = 0; i < 5; i++) {
            log.push(obs::Event("test" + std::to_string(i), nlohmann::json{{"value", i}}));
        }
        
        auto events = log.getRecentEvents(10);
        CHECK(events.size() == 3); // Should only have last 3 events
        CHECK(events[0].type == "test2"); // Oldest in buffer
        CHECK(events[2].type == "test4"); // Newest in buffer
    }
    
    TEST_CASE("Search functionality") {
        obs::EventLog log(10);
        
        log.push(obs::Event("scene_changed", nlohmann::json{{"scene", "main"}}));
        log.push(obs::Event("stream_started", nlohmann::json{{"status", "active"}}));
        log.push(obs::Event("scene_changed", nlohmann::json{{"scene", "break"}}));
        
        auto sceneEvents = log.getEventsByType("scene_changed");
        CHECK(sceneEvents.size() == 2);
        
        auto searchResults = log.searchEvents("scene");
        CHECK(searchResults.size() == 2);
    }
    
    TEST_CASE("Save and load JSON") {
        obs::EventLog log(10);
        
        log.push(obs::Event("test1", nlohmann::json{{"value", 1}}));
        log.push(obs::Event("test2", nlohmann::json{{"value", 2}}));
        
        // Save to file
        std::string testFile = "test_events.json";
        CHECK(log.saveToJson(testFile));
        
        // Load from file
        obs::EventLog newLog(10);
        CHECK(newLog.loadFromJson(testFile));
        
        auto events = newLog.getRecentEvents(10);
        CHECK(events.size() == 2);
        CHECK(events[0].type == "test1");
        CHECK(events[1].type == "test2");
        
        // Clean up
        std::remove(testFile.c_str());
    }
}
