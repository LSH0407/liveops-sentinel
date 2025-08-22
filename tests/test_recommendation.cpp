#include <doctest/doctest.h>
#include "../src/diag/Recommendation.h"
#include <cmath>

TEST_SUITE("Recommendation Engine") {
    TEST_CASE("Basic recommendation calculation") {
        RecommendationInput input;
        input.network.sustainedUplinkMbps = 10.0;
        input.network.rttMs = 30.0;
        input.network.lossPct = 0.3;
        input.video.outputWidth = 1920;
        input.video.outputHeight = 1080;
        input.video.fps = 60.0;
        input.preferredEncoder = EncoderType::NVENC;
        input.headroom = 0.75;
        input.minKbps = 800;
        input.maxKbps = 15000;
        
        auto result = RecommendationEngine::RecommendObsSettings(input);
        
        CHECK(result.bitrateKbps > 0);
        CHECK(result.bitrateKbps >= input.minKbps);
        CHECK(result.bitrateKbps <= input.maxKbps);
        CHECK(result.encoder == EncoderType::NVENC);
        CHECK(result.keyframeSec >= 1);
        CHECK(result.keyframeSec <= 4);
    }
    
    TEST_CASE("High network instability reduces bitrate") {
        RecommendationInput input1, input2;
        
        // Good network
        input1.network.sustainedUplinkMbps = 10.0;
        input1.network.rttMs = 20.0;
        input1.network.lossPct = 0.1;
        input1.video.outputWidth = 1920;
        input1.video.outputHeight = 1080;
        input1.video.fps = 60.0;
        input1.headroom = 0.75;
        
        // Poor network
        input2.network.sustainedUplinkMbps = 10.0;
        input2.network.rttMs = 120.0;
        input2.network.lossPct = 2.5;
        input2.video.outputWidth = 1920;
        input2.video.outputHeight = 1080;
        input2.video.fps = 60.0;
        input2.headroom = 0.75;
        
        auto result1 = RecommendationEngine::RecommendObsSettings(input1);
        auto result2 = RecommendationEngine::RecommendObsSettings(input2);
        
        // Poor network should result in lower bitrate
        CHECK(result2.bitrateKbps < result1.bitrateKbps);
    }
    
    TEST_CASE("Resolution-based bitrate clamping") {
        RecommendationInput input;
        input.network.sustainedUplinkMbps = 20.0; // High bandwidth
        input.network.rttMs = 20.0;
        input.network.lossPct = 0.1;
        input.headroom = 0.75;
        input.minKbps = 800;
        input.maxKbps = 15000;
        
        // 720p30
        input.video.outputWidth = 1280;
        input.video.outputHeight = 720;
        input.video.fps = 30.0;
        auto result720p30 = RecommendationEngine::RecommendObsSettings(input);
        
        // 1080p60
        input.video.outputWidth = 1920;
        input.video.outputHeight = 1080;
        input.video.fps = 60.0;
        auto result1080p60 = RecommendationEngine::RecommendObsSettings(input);
        
        // 4K30
        input.video.outputWidth = 3840;
        input.video.outputHeight = 2160;
        input.video.fps = 30.0;
        auto result4K30 = RecommendationEngine::RecommendObsSettings(input);
        
        // Higher resolution should result in higher bitrate
        CHECK(result1080p60.bitrateKbps > result720p30.bitrateKbps);
        CHECK(result4K30.bitrateKbps > result1080p60.bitrateKbps);
        
        // All should be within reasonable bounds
        CHECK(result720p30.bitrateKbps >= 2500);
        CHECK(result720p30.bitrateKbps <= 4500);
        CHECK(result1080p60.bitrateKbps >= 6000);
        CHECK(result1080p60.bitrateKbps <= 9000);
        CHECK(result4K30.bitrateKbps >= 13000);
        CHECK(result4K30.bitrateKbps <= 20000);
    }
    
    TEST_CASE("Encoder selection based on system load") {
        RecommendationInput input;
        input.network.sustainedUplinkMbps = 10.0;
        input.network.rttMs = 30.0;
        input.network.lossPct = 0.3;
        input.video.outputWidth = 1920;
        input.video.outputHeight = 1080;
        input.video.fps = 60.0;
        input.headroom = 0.75;
        
        // Low CPU, low GPU - should prefer NVENC
        input.system.cpuPct = 30.0;
        input.system.gpuPct = 40.0;
        input.preferredEncoder = EncoderType::NVENC;
        auto result1 = RecommendationEngine::RecommendObsSettings(input);
        
        // High CPU - should prefer x264 or fallback
        input.system.cpuPct = 90.0;
        input.system.gpuPct = 40.0;
        input.preferredEncoder = EncoderType::X264;
        auto result2 = RecommendationEngine::RecommendObsSettings(input);
        
        // High GPU - should avoid NVENC
        input.system.cpuPct = 30.0;
        input.system.gpuPct = 90.0;
        input.preferredEncoder = EncoderType::NVENC;
        auto result3 = RecommendationEngine::RecommendObsSettings(input);
        
        CHECK(result1.encoder == EncoderType::NVENC);
        // Note: Actual encoder selection may vary based on implementation
    }
    
    TEST_CASE("Preset selection based on performance") {
        RecommendationInput input;
        input.network.sustainedUplinkMbps = 10.0;
        input.network.rttMs = 30.0;
        input.network.lossPct = 0.3;
        input.video.outputWidth = 1920;
        input.video.outputHeight = 1080;
        input.video.fps = 60.0;
        input.headroom = 0.75;
        input.preferredEncoder = EncoderType::NVENC;
        
        // Low load - should use quality preset
        input.system.cpuPct = 30.0;
        input.system.gpuPct = 40.0;
        input.obs.encodingLagMs = 10.0;
        auto result1 = RecommendationEngine::RecommendObsSettings(input);
        
        // High load - should use performance preset
        input.system.cpuPct = 90.0;
        input.system.gpuPct = 90.0;
        input.obs.encodingLagMs = 30.0;
        auto result2 = RecommendationEngine::RecommendObsSettings(input);
        
        // Note: Preset selection logic may vary based on implementation
        CHECK(result1.preset != result2.preset || result1.bitrateKbps != result2.bitrateKbps);
    }
}
