#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <unordered_map>

namespace core {

enum class CacheEvictionPolicy {
    LRU,    // Least Recently Used
    LFU,    // Least Frequently Used
    FIFO    // First In First Out
};

struct MemoryUsage {
    size_t current_usage_bytes;
    size_t peak_usage_bytes;
    size_t allocated_blocks;
    size_t freed_blocks;
    double fragmentation_ratio;
    double usage_percent;
    size_t used_bytes;
    std::chrono::system_clock::time_point timestamp;
    
    MemoryUsage() : current_usage_bytes(0), peak_usage_bytes(0), 
                    allocated_blocks(0), freed_blocks(0), fragmentation_ratio(0.0),
                    usage_percent(0.0), used_bytes(0) {}
};

struct MemoryLeak {
    std::string file;
    int line;
    size_t size;
    std::chrono::system_clock::time_point allocation_time;
    std::string stack_trace;
    
    MemoryLeak() : line(0), size(0) {}
};

struct CacheEntry {
    std::string key;
    std::vector<uint8_t> data;
    std::chrono::system_clock::time_point last_access;
    size_t access_count;
    size_t size_bytes;
    
    CacheEntry() : access_count(0), size_bytes(0) {}
};

class MemoryManager {
public:
    static MemoryManager& getInstance();
    
    // 메모리 모니터링
    MemoryUsage getCurrentUsage() const;
    std::vector<MemoryLeak> detectLeaks() const;
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // 메모리 최적화
    void optimizeMemory();
    void clearCache();
    void defragmentMemory();
    void setMemoryLimit(size_t limit_bytes);
    size_t getMemoryLimit() const;
    
    // 캐시 관리
    bool addToCache(const std::string& key, const std::vector<uint8_t>& data);
    std::vector<uint8_t> getFromCache(const std::string& key);
    void removeFromCache(const std::string& key);
    void clearExpiredCache();
    size_t getCacheSize() const;
    size_t getCacheHitRate() const;
    
    // 메모리 할당 추적
    void* trackAllocation(size_t size, const char* file = nullptr, int line = 0);
    void trackDeallocation(void* ptr);
    void enableTracking(bool enabled);
    bool isTrackingEnabled() const;
    
    // 가비지 컬렉션
    void runGarbageCollection();
    void setGarbageCollectionInterval(std::chrono::seconds interval);
    std::chrono::seconds getGarbageCollectionInterval() const;
    
    // 메모리 통계
    struct MemoryStats {
        size_t total_allocated;
        size_t total_freed;
        size_t current_allocated;
        size_t peak_allocated;
        size_t allocation_count;
        size_t deallocation_count;
        double average_allocation_size;
        double fragmentation_percentage;
        
        MemoryStats() : total_allocated(0), total_freed(0), current_allocated(0),
                       peak_allocated(0), allocation_count(0), deallocation_count(0),
                       average_allocation_size(0.0), fragmentation_percentage(0.0) {}
    };
    
    MemoryStats getStats() const;
    void resetStats();
    
    // 콜백 설정
    using MemoryCallback = std::function<void(const MemoryUsage&)>;
    void setMemoryThresholdCallback(size_t threshold_bytes, MemoryCallback callback);
    void setLeakDetectionCallback(std::function<void(const std::vector<MemoryLeak>&)> callback);
    
    // 설정 관리
    void setMaxCacheSize(size_t max_size_bytes);
    void setCacheExpirationTime(std::chrono::minutes expiration);
    void setLeakDetectionEnabled(bool enabled);
    void setAutoOptimizationEnabled(bool enabled);
    void enableAutoOptimization(bool enabled);
    void setCacheEvictionPolicy(CacheEvictionPolicy policy);
    
    // 메모리 사용량 조회 (별칭)
    MemoryUsage getMemoryUsage() const { return getCurrentUsage(); }
    
    // 유틸리티
    std::string formatBytes(size_t bytes) const;
    double getMemoryUsagePercentage() const;
    bool isMemoryPressure() const;
    
private:
    MemoryManager();
    ~MemoryManager();
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    void monitoringLoop();
    void garbageCollectionLoop();
    void checkMemoryThresholds();
    void updateStats();
    void cleanupExpiredCache();
    void evictCacheIfNeeded();
    void updateLRU(const std::string& key);
    std::string getStackTrace() const;
    
    // 멤버 변수
    mutable std::mutex mutex_;
    std::atomic<bool> monitoring_enabled_;
    std::atomic<bool> tracking_enabled_;
    std::atomic<bool> leak_detection_enabled_;
    std::atomic<bool> auto_optimization_enabled_;
    std::atomic<bool> running_;
    
    std::thread monitoring_thread_;
    std::thread gc_thread_;
    std::condition_variable gc_cv_;
    
    // 메모리 통계
    mutable std::mutex stats_mutex_;
    MemoryStats stats_;
    MemoryUsage current_usage_;
    
    // 메모리 추적
    std::map<void*, std::pair<size_t, std::chrono::system_clock::time_point>> allocations_;
    std::vector<MemoryLeak> detected_leaks_;
    
    // 캐시 관리
    std::map<std::string, CacheEntry> cache_;
    std::list<std::string> lru_list_;  // LRU 리스트
    std::unordered_map<std::string, std::list<std::string>::iterator> lru_map_;  // LRU 맵
    size_t max_cache_size_;
    std::chrono::minutes cache_expiration_;
    size_t cache_hits_;
    size_t cache_misses_;
    CacheEvictionPolicy cache_eviction_policy_;
    
    // 메모리 제한
    size_t memory_limit_;
    std::chrono::seconds gc_interval_;
    
    // 콜백
    MemoryCallback threshold_callback_;
    std::function<void(const std::vector<MemoryLeak>&)> leak_callback_;
    size_t threshold_bytes_;
};

// 전역 메모리 할당 추적 매크로
#ifdef _DEBUG
    #define TRACK_ALLOCATION(size) core::MemoryManager::getInstance().trackAllocation(size, __FILE__, __LINE__)
    #define TRACK_DEALLOCATION(ptr) core::MemoryManager::getInstance().trackDeallocation(ptr)
#else
    #define TRACK_ALLOCATION(size) nullptr
    #define TRACK_DEALLOCATION(ptr) (void)0
#endif

} // namespace core
