#pragma once

#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <future>
#include <list>

namespace core {

struct CpuUsage {
    double total_usage_percent;
    double user_usage_percent;
    double system_usage_percent;
    double idle_percent;
    int core_count;
    std::vector<double> per_core_usage;
    std::chrono::system_clock::time_point timestamp;
    
    CpuUsage() : total_usage_percent(0.0), user_usage_percent(0.0), 
                 system_usage_percent(0.0), idle_percent(0.0), core_count(0) {}
};

struct ThreadInfo {
    std::string name;
    std::thread::id id;
    double cpu_usage_percent;
    size_t memory_usage_bytes;
    std::chrono::system_clock::time_point last_active;
    bool is_active;
    
    ThreadInfo() : cpu_usage_percent(0.0), memory_usage_bytes(0), is_active(false) {}
};

struct TaskInfo {
    std::string name;
    std::string description;
    std::chrono::microseconds execution_time;
    std::chrono::microseconds average_time;
    size_t execution_count;
    double cpu_usage_percent;
    size_t memory_usage_bytes;
    std::chrono::system_clock::time_point last_execution;
    
    TaskInfo() : execution_time(0), average_time(0), execution_count(0),
                 cpu_usage_percent(0.0), memory_usage_bytes(0) {}
};

struct PerformanceProfile {
    std::string name;
    double cpu_threshold_percent;
    double memory_threshold_mb;
    std::chrono::milliseconds max_execution_time;
    bool auto_optimize;
    std::vector<std::string> optimization_rules;
    
    PerformanceProfile() : cpu_threshold_percent(75.0), memory_threshold_mb(50.0),
                          max_execution_time(std::chrono::milliseconds(50)), auto_optimize(true) {}
};

struct ScheduledTaskItem {
    std::chrono::system_clock::time_point scheduled_time;
    std::string name;
    std::function<void()> task;
    
    ScheduledTaskItem(const std::chrono::system_clock::time_point& time, 
                     const std::string& n, std::function<void()> t)
        : scheduled_time(time), name(n), task(t) {}
    
    bool operator>(const ScheduledTaskItem& other) const {
        return scheduled_time > other.scheduled_time;
    }
};

class PerformanceOptimizer {
public:
    static PerformanceOptimizer& getInstance();
    
    // CPU 모니터링
    CpuUsage getCurrentCpuUsage() const;
    std::vector<ThreadInfo> getThreadInfo() const;
    void startCpuMonitoring();
    void stopCpuMonitoring();
    bool isCpuMonitoring() const;
    
    // 성능 프로파일링
    void startProfiling(const std::string& task_name);
    void endProfiling(const std::string& task_name);
    TaskInfo getTaskInfo(const std::string& task_name) const;
    std::vector<TaskInfo> getAllTaskInfo() const;
    void resetProfiling();
    
    // 스레드 풀 관리
    void setThreadPoolSize(size_t size);
    size_t getThreadPoolSize() const;
    std::future<void> submitTask(std::function<void()> task);
    std::future<void> submitTask(const std::string& name, std::function<void()> task);
    void waitForAllTasks();
    
    // 비동기 처리
    template<typename F, typename... Args>
    auto async(F&& f, Args&&... args) -> std::future<decltype(f(std::forward<Args>(args)...))>;
    
    // 성능 최적화
    void optimizePerformance();
    void setPerformanceProfile(const PerformanceProfile& profile);
    PerformanceProfile getPerformanceProfile() const;
    void enableAutoOptimization(bool enabled);
    bool isAutoOptimizationEnabled() const;
    
    // 작업 스케줄링
    void scheduleTask(const std::string& name, std::function<void()> task, 
                     std::chrono::milliseconds delay);
    void cancelScheduledTask(const std::string& name);
    void cancelAllScheduledTasks();
    
    // 성능 통계
    struct PerformanceStats {
        double average_cpu_usage;
        double peak_cpu_usage;
        double average_memory_usage_mb;
        double peak_memory_usage_mb;
        size_t total_tasks_executed;
        size_t tasks_per_second;
        std::chrono::microseconds average_task_time;
        std::chrono::microseconds slowest_task_time;
        
        PerformanceStats() : average_cpu_usage(0.0), peak_cpu_usage(0.0),
                            average_memory_usage_mb(0.0), peak_memory_usage_mb(0.0),
                            total_tasks_executed(0), tasks_per_second(0),
                            average_task_time(0), slowest_task_time(0) {}
    };
    
    PerformanceStats getStats() const;
    void resetStats();
    
    // 콜백 설정
    void setCpuThresholdCallback(double threshold, std::function<void(const CpuUsage&)> callback);
    void setTaskTimeoutCallback(std::function<void(const std::string&, std::chrono::milliseconds)> callback);
    
    // 설정 관리
    void setCpuThreshold(double threshold);
    void setMemoryThreshold(size_t threshold_mb);
    void setTaskTimeout(std::chrono::milliseconds timeout);
    void setOptimizationInterval(std::chrono::seconds interval);
    
    // 유틸리티
    bool isCpuOverloaded() const;
    bool isMemoryOverloaded() const;
    double getCpuEfficiency() const;
    std::string getPerformanceReport() const;
    
private:
    PerformanceOptimizer();
    ~PerformanceOptimizer();
    PerformanceOptimizer(const PerformanceOptimizer&) = delete;
    PerformanceOptimizer& operator=(const PerformanceOptimizer&) = delete;
    
    // 모니터링 및 최적화 루프
    void monitoringLoop();
    void optimizationLoop();
    void taskSchedulerLoop();
    
    // CPU 및 스레드 관리
    void updateCpuUsage();
    void updateThreadInfo();
    void adjustThreadPoolSize();
    void resizeThreadPool(size_t new_size);
    
    // 최적화 함수들
    void performOptimization();
    void optimizeHighCpuUsage();
    void optimizeMemoryPressure();
    void checkTaskTimeouts();
    void schedulePriorityTasks();
    void deferLowPriorityTasks();
    void clearCaches();
    void cancelNonEssentialTasks();
    
    // 헬퍼 함수들
    void workerThreadFunction();
    void executeTask(const std::string& name, std::function<void()> task);
    double calculateCpuUsage();
    bool isMemoryPressure() const;
    
    // 멤버 변수
    mutable std::mutex mutex_;
    std::atomic<bool> cpu_monitoring_enabled_;
    std::atomic<bool> auto_optimization_enabled_;
    std::atomic<bool> running_;
    std::atomic<bool> stop_workers_;
    
    // 스레드들
    std::thread monitoring_thread_;
    std::thread optimization_thread_;
    std::thread scheduler_thread_;
    std::vector<std::thread> worker_threads_;
    
    // CPU 모니터링
    mutable std::mutex cpu_mutex_;
    CpuUsage current_cpu_usage_;
    std::vector<ThreadInfo> thread_info_;
    
    // 성능 프로파일링
    std::map<std::string, TaskInfo> task_info_;
    std::map<std::string, std::chrono::high_resolution_clock::time_point> active_profiles_;
    
    // 스레드 풀 및 작업 큐
    std::queue<std::packaged_task<void()>> task_queue_;
    std::mutex task_queue_mutex_;
    std::condition_variable task_cv_;
    size_t thread_pool_size_;
    size_t max_task_queue_size_;
    bool adaptive_thread_pool_;
    
    // 작업 스케줄링
    std::priority_queue<ScheduledTaskItem, std::vector<ScheduledTaskItem>, 
                       std::greater<ScheduledTaskItem>> scheduled_tasks_;
    std::mutex scheduled_tasks_mutex_;
    std::mutex scheduler_mutex_;
    std::condition_variable scheduler_cv_;
    
    // 성능 통계
    mutable std::mutex stats_mutex_;
    PerformanceStats stats_;
    PerformanceProfile current_profile_;
    
    // 임계값 및 간격 설정
    double cpu_threshold_;
    size_t memory_threshold_mb_;
    std::chrono::milliseconds task_timeout_;
    std::chrono::seconds optimization_interval_;
    std::chrono::milliseconds monitoring_interval_;
    
    // 콜백 함수들
    std::function<void(const CpuUsage&)> cpu_threshold_callback_;
    std::function<void(const std::string&, std::chrono::milliseconds)> task_timeout_callback_;
};

// 비동기 처리 템플릿 구현
template<typename F, typename... Args>
auto PerformanceOptimizer::async(F&& f, Args&&... args) -> std::future<decltype(f(std::forward<Args>(args)...))> {
    using return_type = decltype(f(std::forward<Args>(args)...));
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        if (task_queue_.size() >= max_task_queue_size_) {
            throw std::runtime_error("Task queue full");
        }
        task_queue_.push([task]() { (*task)(); });
    }
    
    task_cv_.notify_one();
    return result;
}

// 성능 프로파일링 매크로
#ifdef _DEBUG
    #define PROFILE_START(name) core::PerformanceOptimizer::getInstance().startProfiling(name)
    #define PROFILE_END(name) core::PerformanceOptimizer::getInstance().endProfiling(name)
    #define PROFILE_SCOPE(name) auto profile_scope_##name = core::ProfileScope(name)
#else
    #define PROFILE_START(name) (void)0
    #define PROFILE_END(name) (void)0
    #define PROFILE_SCOPE(name) (void)0
#endif

// 자동 프로파일링 스코프 클래스
class ProfileScope {
public:
    ProfileScope(const std::string& name) : name_(name) {
        PerformanceOptimizer::getInstance().startProfiling(name_);
    }
    
    ~ProfileScope() {
        PerformanceOptimizer::getInstance().endProfiling(name_);
    }
    
private:
    std::string name_;
};

} // namespace core
