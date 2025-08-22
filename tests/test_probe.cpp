#include <doctest/doctest.h>
#include "../src/net/Probe.h"
#include <thread>
#include <chrono>

TEST_CASE("Probe - Local Echo Server") {
    SUBCASE("Start and stop local echo server") {
        int testPort = 50052;
        
        // Start echo server
        bool startResult = Probe::startLocalEcho(testPort);
        CHECK(startResult == true);
        
        // Give some time for server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Stop echo server
        Probe::stopLocalEcho();
        
        // Give some time for server to stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    SUBCASE("Multiple start/stop cycles") {
        int testPort = 50053;
        
        for (int i = 0; i < 3; ++i) {
            bool startResult = Probe::startLocalEcho(testPort);
            CHECK(startResult == true);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            Probe::stopLocalEcho();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

TEST_CASE("Probe - Basic Functionality") {
    Probe probe;
    
    SUBCASE("Start and stop probe") {
        bool callbackCalled = false;
        double lastRtt = 0.0;
        double lastLoss = 0.0;
        
        auto callback = [&](double rtt, double loss) {
            callbackCalled = true;
            lastRtt = rtt;
            lastLoss = loss;
        };
        
        // Start local echo server for testing
        Probe::startLocalEcho(50054);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Start probe
        bool startResult = probe.start("127.0.0.1", 50054, 10, callback);
        CHECK(startResult == true);
        
        // Wait for some measurements
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        // Stop probe
        probe.stop();
        
        // Stop echo server
        Probe::stopLocalEcho();
        
        // Verify callback was called
        CHECK(callbackCalled == true);
        CHECK(lastRtt > 0.0);
        CHECK(lastLoss >= 0.0);
    }
    
    SUBCASE("Probe with invalid target") {
        bool callbackCalled = false;
        
        auto callback = [&](double rtt, double loss) {
            callbackCalled = true;
        };
        
        // Start probe with invalid target
        bool startResult = probe.start("invalid.host.local", 12345, 1, callback);
        CHECK(startResult == true); // Should still start, but won't get responses
        
        // Wait a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Stop probe
        probe.stop();
        
        // Callback might not be called due to no responses
        // This is expected behavior
    }
}
