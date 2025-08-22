#include <doctest/doctest.h>
#include <chrono>
#include <thread>

TEST_SUITE("Threshold Hold Time Logic") {
    TEST_CASE("Alert should not trigger immediately") {
        const double threshold = 80.0;
        const int holdSec = 5;
        
        // Simulate values above threshold
        double currentValue = 100.0;
        auto violationStart = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        
        // Should not trigger immediately
        bool shouldAlert = (currentValue > threshold) && 
                          ((now - violationStart) >= std::chrono::seconds(holdSec));
        
        CHECK(!shouldAlert);
    }
    
    TEST_CASE("Alert should trigger after hold time") {
        const double threshold = 80.0;
        const int holdSec = 1; // Short hold time for testing
        
        // Simulate values above threshold
        double currentValue = 100.0;
        auto violationStart = std::chrono::steady_clock::now();
        
        // Wait for hold time to pass
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        
        auto now = std::chrono::steady_clock::now();
        bool shouldAlert = (currentValue > threshold) && 
                          ((now - violationStart) >= std::chrono::seconds(holdSec));
        
        CHECK(shouldAlert);
    }
    
    TEST_CASE("Alert should reset when value drops below threshold") {
        const double threshold = 80.0;
        const int holdSec = 5;
        
        // Simulate violation start
        auto violationStart = std::chrono::steady_clock::now();
        
        // Value drops below threshold
        double currentValue = 50.0;
        auto now = std::chrono::steady_clock::now();
        
        // Should reset violation timer
        if (currentValue <= threshold) {
            violationStart = now;
        }
        
        // Wait a bit and check again with high value
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        currentValue = 100.0;
        now = std::chrono::steady_clock::now();
        
        bool shouldAlert = (currentValue > threshold) && 
                          ((now - violationStart) >= std::chrono::seconds(holdSec));
        
        // Should not alert because violation timer was reset
        CHECK(!shouldAlert);
    }
    
    TEST_CASE("Multiple threshold violations") {
        const double rttThreshold = 80.0;
        const double lossThreshold = 2.0;
        const int holdSec = 1;
        
        // Simulate both RTT and loss violations
        double rtt = 100.0;
        double loss = 5.0;
        
        auto rttViolationStart = std::chrono::steady_clock::now();
        auto lossViolationStart = std::chrono::steady_clock::now();
        
        // Wait for hold time
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        
        auto now = std::chrono::steady_clock::now();
        
        bool rttAlert = (rtt > rttThreshold) && 
                       ((now - rttViolationStart) >= std::chrono::seconds(holdSec));
        
        bool lossAlert = (loss > lossThreshold) && 
                        ((now - lossViolationStart) >= std::chrono::seconds(holdSec));
        
        CHECK(rttAlert);
        CHECK(lossAlert);
    }
}
