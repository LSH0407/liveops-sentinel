#include <doctest/doctest.h>
#include "../src/core/Metrics.h"

TEST_CASE("EMA - Exponential Moving Average") {
    EMA ema(0.5);
    
    SUBCASE("Initial value") {
        CHECK(ema.value() == 0.0);
    }
    
    SUBCASE("First value") {
        double result = ema.push(10.0);
        CHECK(result == 10.0);
        CHECK(ema.value() == 10.0);
    }
    
    SUBCASE("Multiple values") {
        ema.push(10.0);
        ema.push(20.0);
        double result = ema.push(30.0);
        
        // EMA = 0.5 * 30 + 0.5 * (0.5 * 20 + 0.5 * 10) = 15 + 7.5 = 22.5
        CHECK(result == doctest::Approx(22.5));
    }
}

TEST_CASE("MetricsCollector - Statistics Collection") {
    MetricsCollector collector(5);
    
    SUBCASE("Empty collector") {
        CHECK(collector.getSampleCount() == 0);
        CHECK(collector.getAverage() == 0.0);
        CHECK(collector.getMin() == 0.0);
        CHECK(collector.getMax() == 0.0);
        CHECK(collector.getStdDev() == 0.0);
    }
    
    SUBCASE("Single sample") {
        collector.addSample(10.0);
        CHECK(collector.getSampleCount() == 1);
        CHECK(collector.getAverage() == 10.0);
        CHECK(collector.getMin() == 10.0);
        CHECK(collector.getMax() == 10.0);
        CHECK(collector.getStdDev() == 0.0);
    }
    
    SUBCASE("Multiple samples") {
        collector.addSample(10.0);
        collector.addSample(20.0);
        collector.addSample(30.0);
        
        CHECK(collector.getSampleCount() == 3);
        CHECK(collector.getAverage() == doctest::Approx(20.0));
        CHECK(collector.getMin() == 10.0);
        CHECK(collector.getMax() == 30.0);
        CHECK(collector.getStdDev() > 0.0);
    }
    
    SUBCASE("Max samples limit") {
        for (int i = 0; i < 10; ++i) {
            collector.addSample(static_cast<double>(i));
        }
        
        CHECK(collector.getSampleCount() == 5); // maxSamples = 5
        CHECK(collector.getMin() == 5.0); // oldest samples removed
        CHECK(collector.getMax() == 9.0);
    }
    
    SUBCASE("Clear samples") {
        collector.addSample(10.0);
        collector.addSample(20.0);
        collector.clear();
        
        CHECK(collector.getSampleCount() == 0);
        CHECK(collector.getAverage() == 0.0);
    }
}
