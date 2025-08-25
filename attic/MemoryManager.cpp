#include "MemoryManager.h"
#include "Logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <unordered_map>
#include <list>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace core {

MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

MemoryManager::MemoryManager() 
    : monitoring_enabled_(false)
    , tracking_enabled_(false)
    , leak_detection_enabled_(false)
    , auto_optimization_enabled_(false)
    , running_(false)
    , max_cache_size_(50 * 1024 * 1024) // 50MB로 줄임
    , cache_expiration_(std::chrono::minutes(15)) // 15분으로 줄임
    , cache_hits_(0)
    , cache_misses_(0)
    , memory_limit_(0)
    , gc_interval_(std::chrono::seconds(30)) // 30초로 줄임
    , threshold_bytes_(0)
    , cache_eviction_policy_(CacheEvictionPolicy::LRU) {
}

MemoryManager::~MemoryManager() {
    stopMonitoring();
}

MemoryUsage MemoryManager::getCurrentUsage() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    MemoryUsage usage;
    
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        usage.current_usage_bytes = pmc.WorkingSetSize;
        usage.peak_usage_bytes = pmc.PeakWorkingSetSize;
        usage.used_bytes = pmc.WorkingSetSize;
        usage.usage_percent = (double)pmc.WorkingSetSize / (double)pmc.PagefileUsage * 100.0;
    }
#else
    // Linux에서 메모리 사용량 확인
    FILE* file = fopen("/proc/self/status", "r");
    if (file) {
        char line[128];
        size_t total_memory = 0;
        while (fgets(line, 128, file) != NULL) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                usage.current_usage_bytes = atoi(line + 6) * 1024; // KB to bytes
                usage.used_bytes = usage.current_usage_bytes;
            } else if (strncmp(line, "VmSize:", 7) == 0) {
                total_memory = atoi(line + 7) * 1024; // KB to bytes
            }
        }
        fclose(file);
        if (total_memory > 0) {
            usage.usage_percent = (double)usage.used_bytes / (double)total_memory * 100.0;
        }
    }
#endif
    
    usage.timestamp = std::chrono::system_clock::now();
    return usage;
}

std::vector<MemoryLeak> MemoryManager::detectLeaks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MemoryLeak> leaks;
    
    auto now = std::chrono::system_clock::now();
    for (const auto& alloc : allocations_) {
        // 5분 이상 할당된 메모리를 누수로 간주
        if (now - alloc.second.second > std::chrono::minutes(5)) {
            MemoryLeak leak;
            leak.size = alloc.second.first;
            leak.allocation_time = alloc.second.second;
            leak.stack_trace = getStackTrace();
            leaks.push_back(leak);
        }
    }
    
    return leaks;
}

void MemoryManager::startMonitoring() {
    if (monitoring_enabled_) return;
    
    monitoring_enabled_ = true;
    running_ = true;
    
    monitoring_thread_ = std::thread(&MemoryManager::monitoringLoop, this);
    gc_thread_ = std::thread(&MemoryManager::garbageCollectionLoop, this);
    
    Logger::getInstance().info("메모리 모니터링 시작");
}

void MemoryManager::stopMonitoring() {
    if (!monitoring_enabled_) return;
    
    running_ = false;
    monitoring_enabled_ = false;
    gc_cv_.notify_all();
    
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    if (gc_thread_.joinable()) {
        gc_thread_.join();
    }
    
    Logger::getInstance().info("메모리 모니터링 중지");
}

bool MemoryManager::isMonitoring() const {
    return monitoring_enabled_;
}

void MemoryManager::optimizeMemory() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 캐시 정리 (더 적극적으로)
    clearExpiredCache();
    evictCacheIfNeeded();
    
    // 메모리 조각화 해결
    defragmentMemory();
    
    // 가비지 컬렉션 실행
    runGarbageCollection();
    
    Logger::getInstance().info("메모리 최적화 완료");
}

void MemoryManager::clearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    lru_list_.clear();
    cache_hits_ = 0;
    cache_misses_ = 0;
    
    Logger::getInstance().info("캐시 정리 완료");
}

void MemoryManager::defragmentMemory() {
    // 메모리 조각화 해결을 위한 간단한 구현
    // 실제로는 더 복잡한 알고리즘이 필요
    Logger::getInstance().debug("메모리 조각화 해결 시도");
}

void MemoryManager::setMemoryLimit(size_t limit_bytes) {
    memory_limit_ = limit_bytes;
    Logger::getInstance().info("메모리 제한 설정: {} bytes", limit_bytes);
}

size_t MemoryManager::getMemoryLimit() const {
    return memory_limit_;
}

bool MemoryManager::addToCache(const std::string& key, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 캐시 크기 제한 확인
    if (getCacheSize() + data.size() > max_cache_size_) {
        clearExpiredCache();
        evictCacheIfNeeded();
        if (getCacheSize() + data.size() > max_cache_size_) {
            return false;
        }
    }
    
    CacheEntry entry;
    entry.key = key;
    entry.data = data;
    entry.last_access = std::chrono::system_clock::now();
    entry.access_count = 1;
    entry.size_bytes = data.size();
    
    cache_[key] = entry;
    
    // LRU 리스트에 추가
    lru_list_.push_front(key);
    lru_map_[key] = lru_list_.begin();
    
    return true;
}

std::vector<uint8_t> MemoryManager::getFromCache(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second.last_access = std::chrono::system_clock::now();
        it->second.access_count++;
        cache_hits_++;
        
        // LRU 업데이트
        updateLRU(key);
        
        return it->second.data;
    }
    
    cache_misses_++;
    return std::vector<uint8_t>();
}

void MemoryManager::removeFromCache(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.erase(key);
    
    // LRU 리스트에서도 제거
    auto lru_it = lru_map_.find(key);
    if (lru_it != lru_map_.end()) {
        lru_list_.erase(lru_it->second);
        lru_map_.erase(lru_it);
    }
}

void MemoryManager::clearExpiredCache() {
    auto now = std::chrono::system_clock::now();
    auto it = cache_.begin();
    
    while (it != cache_.end()) {
        if (now - it->second.last_access > cache_expiration_) {
            // LRU 리스트에서도 제거
            auto lru_it = lru_map_.find(it->first);
            if (lru_it != lru_map_.end()) {
                lru_list_.erase(lru_it->second);
                lru_map_.erase(lru_it);
            }
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void MemoryManager::evictCacheIfNeeded() {
    if (getCacheSize() <= max_cache_size_) return;
    
    // LRU 정책에 따라 캐시 항목 제거
    while (getCacheSize() > max_cache_size_ * 0.8 && !lru_list_.empty()) {
        std::string key_to_remove = lru_list_.back();
        removeFromCache(key_to_remove);
    }
}

void MemoryManager::updateLRU(const std::string& key) {
    auto lru_it = lru_map_.find(key);
    if (lru_it != lru_map_.end()) {
        lru_list_.erase(lru_it->second);
        lru_list_.push_front(key);
        lru_map_[key] = lru_list_.begin();
    }
}

size_t MemoryManager::getCacheSize() const {
    size_t total_size = 0;
    for (const auto& entry : cache_) {
        total_size += entry.second.size_bytes;
    }
    return total_size;
}

size_t MemoryManager::getCacheHitRate() const {
    size_t total_requests = cache_hits_ + cache_misses_;
    if (total_requests == 0) return 0;
    return (cache_hits_ * 100) / total_requests;
}

void* MemoryManager::trackAllocation(size_t size, const char* file, int line) {
    if (!tracking_enabled_) return nullptr;
    
    void* ptr = malloc(size);
    if (ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = std::make_pair(size, std::chrono::system_clock::now());
        
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_allocated += size;
        stats_.current_allocated += size;
        stats_.allocation_count++;
        
        if (stats_.current_allocated > stats_.peak_allocated) {
            stats_.peak_allocated = stats_.current_allocated;
        }
    }
    
    return ptr;
}

void MemoryManager::trackDeallocation(void* ptr) {
    if (!tracking_enabled_ || !ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocations_.find(ptr);
    if (it != allocations_.end()) {
        size_t size = it->second.first;
        allocations_.erase(it);
        
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.total_freed += size;
        stats_.current_allocated -= size;
        stats_.deallocation_count++;
    }
    
    free(ptr);
}

void MemoryManager::enableTracking(bool enabled) {
    tracking_enabled_ = enabled;
    Logger::getInstance().info("메모리 추적 {}됨", enabled ? "활성화" : "비활성화");
}

bool MemoryManager::isTrackingEnabled() const {
    return tracking_enabled_;
}

void MemoryManager::runGarbageCollection() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 만료된 캐시 정리
    clearExpiredCache();
    
    // 메모리 누수 검사
    if (leak_detection_enabled_) {
        auto leaks = detectLeaks();
        if (!leaks.empty() && leak_callback_) {
            leak_callback_(leaks);
        }
    }
    
    Logger::getInstance().debug("가비지 컬렉션 완료");
}

void MemoryManager::setGarbageCollectionInterval(std::chrono::seconds interval) {
    gc_interval_ = interval;
}

std::chrono::seconds MemoryManager::getGarbageCollectionInterval() const {
    return gc_interval_;
}

MemoryManager::MemoryStats MemoryManager::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    MemoryStats stats = stats_;
    if (stats.allocation_count > 0) {
        stats.average_allocation_size = static_cast<double>(stats.total_allocated) / stats.allocation_count;
    }
    
    if (stats.peak_allocated > 0) {
        stats.fragmentation_percentage = static_cast<double>(stats.current_allocated) / stats.peak_allocated * 100.0;
    }
    
    return stats;
}

void MemoryManager::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MemoryStats();
    cache_hits_ = 0;
    cache_misses_ = 0;
}

void MemoryManager::setMemoryThresholdCallback(size_t threshold_bytes, MemoryCallback callback) {
    threshold_bytes_ = threshold_bytes;
    threshold_callback_ = callback;
}

void MemoryManager::setLeakDetectionCallback(std::function<void(const std::vector<MemoryLeak>&)> callback) {
    leak_callback_ = callback;
}

void MemoryManager::setMaxCacheSize(size_t max_size_bytes) {
    max_cache_size_ = max_size_bytes;
}

void MemoryManager::setCacheExpirationTime(std::chrono::minutes expiration) {
    cache_expiration_ = expiration;
}

void MemoryManager::setLeakDetectionEnabled(bool enabled) {
    leak_detection_enabled_ = enabled;
}

void MemoryManager::setAutoOptimizationEnabled(bool enabled) {
    auto_optimization_enabled_ = enabled;
}

void MemoryManager::enableAutoOptimization(bool enabled) {
    auto_optimization_enabled_ = enabled;
}

void MemoryManager::setCacheEvictionPolicy(CacheEvictionPolicy policy) {
    cache_eviction_policy_ = policy;
}

std::string MemoryManager::formatBytes(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

double MemoryManager::getMemoryUsagePercentage() const {
    if (memory_limit_ == 0) return 0.0;
    
    auto usage = getCurrentUsage();
    return static_cast<double>(usage.current_usage_bytes) / memory_limit_ * 100.0;
}

bool MemoryManager::isMemoryPressure() const {
    return getMemoryUsagePercentage() > 75.0; // 75%로 낮춤
}

void MemoryManager::monitoringLoop() {
    while (running_) {
        updateStats();
        checkMemoryThresholds();
        
        if (auto_optimization_enabled_ && isMemoryPressure()) {
            optimizeMemory();
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(3)); // 3초로 줄임
    }
}

void MemoryManager::garbageCollectionLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        gc_cv_.wait_for(lock, gc_interval_, [this] { return !running_; });
        
        if (running_) {
            runGarbageCollection();
        }
    }
}

void MemoryManager::checkMemoryThresholds() {
    if (threshold_callback_ && threshold_bytes_ > 0) {
        auto usage = getCurrentUsage();
        if (usage.current_usage_bytes > threshold_bytes_) {
            threshold_callback_(usage);
        }
    }
}

void MemoryManager::updateStats() {
    auto usage = getCurrentUsage();
    current_usage_ = usage;
}

std::string MemoryManager::getStackTrace() const {
    // 간단한 스택 트레이스 구현
    // 실제로는 더 정교한 구현이 필요
    return "Stack trace not implemented";
}

} // namespace core
