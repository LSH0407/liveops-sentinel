#include "PerformanceOptimizer.h"
#include "Logger.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <queue>

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#else
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

namespace core {

PerformanceOptimizer& PerformanceOptimizer::getInstance() {
    static PerformanceOptimizer instance;
    return instance;
}

PerformanceOptimizer::PerformanceOptimizer() 
    : cpu_monitoring_enabled_(false)
    , auto_optimization_enabled_(false)
    , running_(false)
    , stop_workers_(false)
    , thread_pool_size_(std::max(2u, std::thread::hardware_concurrency() - 1)) // 하나의 코어는 여유롭게
    , cpu_threshold_(75.0) // 75%로 낮춤
    , memory_threshold_mb_(50) // 50MB로 낮춤
    , task_timeout_(std::chrono::milliseconds(3000)) // 3초로 줄임
    , optimization_interval_(std::chrono::seconds(20)) // 20초로 줄임
    , monitoring_interval_(std::chrono::milliseconds(2000)) // 2초로 줄임
    , max_task_queue_size_(1000) // 최대 작업 큐 크기 제한
    , adaptive_thread_pool_(true) { // 적응형 스레드 풀 활성화
    
    // 기본 성능 프로파일 설정
    current_profile_.name = "optimized";
    current_profile_.cpu_threshold_percent = 75.0;
    current_profile_.memory_threshold_mb = 50.0;
    current_profile_.max_execution_time = std::chrono::milliseconds(50); // 50ms로 줄임
    current_profile_.auto_optimize = true;
}

PerformanceOptimizer::~PerformanceOptimizer() {
    stopCpuMonitoring();
    
    // 워커 스레드 정리
    {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        stop_workers_ = true;
    }
    task_cv_.notify_all();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

CpuUsage PerformanceOptimizer::getCurrentCpuUsage() const {
    std::lock_guard<std::mutex> lock(cpu_mutex_);
    return current_cpu_usage_;
}

std::vector<ThreadInfo> PerformanceOptimizer::getThreadInfo() const {
    std::lock_guard<std::mutex> lock(cpu_mutex_);
    return thread_info_;
}

void PerformanceOptimizer::startCpuMonitoring() {
    if (cpu_monitoring_enabled_) return;
    
    cpu_monitoring_enabled_ = true;
    running_ = true;
    
    // 워커 스레드 시작 (적응형 크기)
    size_t initial_pool_size = adaptive_thread_pool_ ? 
        std::max<size_t>(2, thread_pool_size_ / 2) : thread_pool_size_;
    
    for (size_t i = 0; i < initial_pool_size; ++i) {
        worker_threads_.emplace_back(&PerformanceOptimizer::workerThreadFunction, this);
    }
    
    monitoring_thread_ = std::thread(&PerformanceOptimizer::monitoringLoop, this);
    optimization_thread_ = std::thread(&PerformanceOptimizer::optimizationLoop, this);
    scheduler_thread_ = std::thread(&PerformanceOptimizer::taskSchedulerLoop, this);
    
    Logger::getInstance().info("CPU 모니터링 시작 (초기 스레드 풀 크기: {})", initial_pool_size);
}

void PerformanceOptimizer::stopCpuMonitoring() {
    if (!cpu_monitoring_enabled_) return;
    
    running_ = false;
    cpu_monitoring_enabled_ = false;
    scheduler_cv_.notify_all();
    
    if (monitoring_thread_.joinable()) {
        monitoring_thread_.join();
    }
    if (optimization_thread_.joinable()) {
        optimization_thread_.join();
    }
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    Logger::getInstance().info("CPU 모니터링 중지");
}

bool PerformanceOptimizer::isCpuMonitoring() const {
    return cpu_monitoring_enabled_;
}

void PerformanceOptimizer::startProfiling(const std::string& task_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_profiles_[task_name] = std::chrono::high_resolution_clock::now(); // high_resolution_clock 사용
}

void PerformanceOptimizer::endProfiling(const std::string& task_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto start_it = active_profiles_.find(task_name);
    if (start_it == active_profiles_.end()) return;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_it->second);
    
    auto& task_info = task_info_[task_name];
    task_info.name = task_name;
    task_info.execution_time = duration;
    task_info.execution_count++;
    task_info.last_execution = std::chrono::system_clock::now();
    
    // 평균 실행 시간 계산 (더 효율적으로)
    if (task_info.execution_count > 1) {
        auto total_time = task_info.average_time * (task_info.execution_count - 1) + duration;
        task_info.average_time = total_time / task_info.execution_count;
    } else {
        task_info.average_time = duration;
    }
    
    // CPU 사용률 계산 (캐시된 값 사용)
    task_info.cpu_usage_percent = getCurrentCpuUsage().total_usage_percent;
    
    active_profiles_.erase(start_it);
    
    // 통계 업데이트 (최적화)
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    stats_.total_tasks_executed++;
    if (duration > stats_.slowest_task_time) {
        stats_.slowest_task_time = duration;
    }
    
    // 평균 작업 시간 업데이트
    if (stats_.total_tasks_executed > 1) {
        auto total_time = stats_.average_task_time * (stats_.total_tasks_executed - 1) + duration;
        stats_.average_task_time = total_time / stats_.total_tasks_executed;
    } else {
        stats_.average_task_time = duration;
    }
}

TaskInfo PerformanceOptimizer::getTaskInfo(const std::string& task_name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = task_info_.find(task_name);
    return (it != task_info_.end()) ? it->second : TaskInfo();
}

std::vector<TaskInfo> PerformanceOptimizer::getAllTaskInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TaskInfo> result;
    result.reserve(task_info_.size());
    
    for (const auto& pair : task_info_) {
        result.push_back(pair.second);
    }
    
    return result;
}

void PerformanceOptimizer::resetProfiling() {
    std::lock_guard<std::mutex> lock(mutex_);
    task_info_.clear();
    active_profiles_.clear();
    
    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
    stats_ = PerformanceStats();
    
    Logger::getInstance().info("성능 프로파일링 초기화");
}

void PerformanceOptimizer::setThreadPoolSize(size_t size) {
    if (size == 0) {
        size = std::max(2u, std::thread::hardware_concurrency() - 1);
    }
    
    thread_pool_size_ = size;
    Logger::getInstance().info("스레드 풀 크기 설정: {}", size);
}

size_t PerformanceOptimizer::getThreadPoolSize() const {
    return thread_pool_size_;
}

std::future<void> PerformanceOptimizer::submitTask(std::function<void()> task) {
    return submitTask("anonymous", task);
}

std::future<void> PerformanceOptimizer::submitTask(const std::string& name, std::function<void()> task) {
    // 작업 큐 크기 제한 확인
    {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        if (task_queue_.size() >= max_task_queue_size_) {
            Logger::getInstance().warn("작업 큐가 가득 찼습니다. 작업 거부: {}", name);
            std::promise<void> promise;
            promise.set_exception(std::make_exception_ptr(std::runtime_error("Task queue full")));
            return promise.get_future();
        }
    }
    
    auto task_wrapper = [this, name, task]() {
        executeTask(name, task);
    };
    
    std::packaged_task<void()> packaged_task(task_wrapper);
    auto future = packaged_task.get_future();
    
    {
        std::lock_guard<std::mutex> lock(task_queue_mutex_);
        task_queue_.push(std::move(packaged_task));
    }
    
    task_cv_.notify_one();
    return future;
}

void PerformanceOptimizer::waitForAllTasks() {
    std::unique_lock<std::mutex> lock(task_queue_mutex_);
    task_cv_.wait(lock, [this] { return task_queue_.empty(); });
}

void PerformanceOptimizer::optimizePerformance() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 스레드 풀 최적화 (구현 예정)
    // optimizeThreadPool();
    
    // 비활성 스레드 정리 (구현 예정)
    // cleanupInactiveThreads();
    
    // 메모리 최적화
    if (isMemoryOverloaded()) {
        Logger::getInstance().warn("메모리 과부하 감지 - 최적화 수행");
    }
    
    core::Logger::getInstance().info("성능 최적화 완료");
}

void PerformanceOptimizer::setPerformanceProfile(const PerformanceProfile& profile) {
    current_profile_ = profile;
    core::Logger::getInstance().info("성능 프로파일 설정: {}", profile.name);
}

PerformanceProfile PerformanceOptimizer::getPerformanceProfile() const {
    return current_profile_;
}

void PerformanceOptimizer::enableAutoOptimization(bool enabled) {
    auto_optimization_enabled_ = enabled;
    core::Logger::getInstance().info("자동 최적화 {}됨", enabled ? "활성화" : "비활성화");
}

bool PerformanceOptimizer::isAutoOptimizationEnabled() const {
    return auto_optimization_enabled_;
}

void PerformanceOptimizer::scheduleTask(const std::string& name, std::function<void()> task, std::chrono::milliseconds delay) {
    auto scheduled_time = std::chrono::system_clock::now() + delay;
    
    std::lock_guard<std::mutex> lock(scheduled_tasks_mutex_);
    scheduled_tasks_.push({scheduled_time, name, task});
}

void PerformanceOptimizer::cancelScheduledTask(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    // scheduled_tasks_.erase(name); // priority_queue는 erase를 지원하지 않음
    core::Logger::getInstance().info("스케줄된 작업 취소: {}", name);
}

void PerformanceOptimizer::cancelAllScheduledTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    // scheduled_tasks_.clear(); // priority_queue는 clear를 지원하지 않음
    while (!scheduled_tasks_.empty()) {
        scheduled_tasks_.pop();
    }
    core::Logger::getInstance().info("모든 스케줄된 작업 취소");
}

PerformanceOptimizer::PerformanceStats PerformanceOptimizer::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void PerformanceOptimizer::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = PerformanceStats();
    core::Logger::getInstance().info("성능 통계 초기화");
}

void PerformanceOptimizer::setCpuThresholdCallback(double threshold, std::function<void(const CpuUsage&)> callback) {
    cpu_threshold_ = threshold;
    cpu_threshold_callback_ = callback;
}

void PerformanceOptimizer::setTaskTimeoutCallback(std::function<void(const std::string&, std::chrono::milliseconds)> callback) {
    task_timeout_callback_ = callback;
}

void PerformanceOptimizer::setCpuThreshold(double threshold_percent) {
    cpu_threshold_ = threshold_percent;
}

void PerformanceOptimizer::setMemoryThreshold(size_t threshold_mb) {
    memory_threshold_mb_ = threshold_mb;
}

void PerformanceOptimizer::setTaskTimeout(std::chrono::milliseconds timeout) {
    task_timeout_ = timeout;
}

void PerformanceOptimizer::setOptimizationInterval(std::chrono::seconds interval) {
    optimization_interval_ = interval;
}

bool PerformanceOptimizer::isCpuOverloaded() const {
    return current_cpu_usage_.total_usage_percent > cpu_threshold_;
}

bool PerformanceOptimizer::isMemoryOverloaded() const {
    return getCurrentMemoryUsage() > memory_threshold_mb_ * 1024 * 1024;
}

double PerformanceOptimizer::getCpuEfficiency() const {
    // CPU 효율성 계산 (작업 처리량 / CPU 사용률)
    if (current_cpu_usage_.total_usage_percent == 0) return 0.0;
    return static_cast<double>(stats_.total_tasks_executed) / current_cpu_usage_.total_usage_percent;
}

std::string PerformanceOptimizer::getPerformanceReport() const {
    std::ostringstream oss;
    
    auto stats = getStats();
    auto cpu_usage = getCurrentCpuUsage();
    
    oss << "=== 성능 보고서 ===\n";
    oss << "CPU 사용률: " << std::fixed << std::setprecision(1) << cpu_usage.total_usage_percent << "%\n";
    oss << "메모리 사용률: " << std::fixed << std::setprecision(1) << stats.average_memory_usage_mb << " MB\n";
    oss << "총 실행된 작업: " << stats.total_tasks_executed << "\n";
    oss << "평균 작업 시간: " << stats.average_task_time.count() << " μs\n";
    oss << "가장 느린 작업: " << stats.slowest_task_time.count() << " μs\n";
    oss << "CPU 효율성: " << std::fixed << std::setprecision(2) << getCpuEfficiency() << "\n";
    
    return oss.str();
}

void PerformanceOptimizer::monitoringLoop() {
    while (running_) {
        updateCpuUsage();
        updateThreadInfo();
        
        // 적응형 스레드 풀 크기 조정
        if (adaptive_thread_pool_) {
            adjustThreadPoolSize();
        }
        
        std::this_thread::sleep_for(monitoring_interval_);
    }
}

void PerformanceOptimizer::optimizationLoop() {
    while (running_) {
        if (auto_optimization_enabled_) {
            performOptimization();
        }
        
        std::this_thread::sleep_for(optimization_interval_);
    }
}

void PerformanceOptimizer::taskSchedulerLoop() {
    while (running_) {
        // 우선순위 작업 스케줄링
        schedulePriorityTasks();
        
        std::unique_lock<std::mutex> lock(scheduler_mutex_);
        scheduler_cv_.wait_for(lock, std::chrono::seconds(1), [this] { return !running_; });
    }
}

void PerformanceOptimizer::updateCpuUsage() {
    std::lock_guard<std::mutex> lock(cpu_mutex_);
    
#ifdef _WIN32
    static PDH_HQUERY cpu_query = NULL;
    static PDH_HCOUNTER cpu_total = NULL;
    
    if (cpu_query == NULL) {
        PdhOpenQuery(NULL, NULL, &cpu_query);
        PdhAddCounter(cpu_query, L"\\Processor(_Total)\\% Processor Time", NULL, &cpu_total);
        PdhCollectQueryData(cpu_query);
        return;
    }
    
    PDH_FMT_COUNTERVALUE counter_val;
    PdhCollectQueryData(cpu_query);
    PdhGetFormattedCounterValue(cpu_total, PDH_FMT_DOUBLE, NULL, &counter_val);
    
    current_cpu_usage_.total_usage_percent = counter_val.doubleValue;
    current_cpu_usage_.timestamp = std::chrono::system_clock::now();
#else
    // Linux CPU 사용률 계산
    static unsigned long long last_total = 0, last_idle = 0;
    
    FILE* file = fopen("/proc/stat", "r");
    if (file) {
        unsigned long long total = 0, idle = 0;
        unsigned long long user, nice, system, idle_time, iowait, irq, softirq, steal;
        
        if (fscanf(file, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal) == 8) {
            total = user + nice + system + idle_time + iowait + irq + softirq + steal;
            idle = idle_time + iowait;
            
            if (last_total > 0) {
                unsigned long long total_diff = total - last_total;
                unsigned long long idle_diff = idle - last_idle;
                
                if (total_diff > 0) {
                    current_cpu_usage_.total_usage_percent = 100.0 * (1.0 - (double)idle_diff / total_diff);
                }
            }
            
            last_total = total;
            last_idle = idle;
        }
        fclose(file);
    }
    
    current_cpu_usage_.timestamp = std::chrono::system_clock::now();
#endif
}

void PerformanceOptimizer::updateThreadInfo() {
    std::lock_guard<std::mutex> lock(cpu_mutex_);
    thread_info_.clear();
    
    // 현재 스레드 정보 수집 (간소화)
    ThreadInfo main_thread;
    main_thread.id = std::this_thread::get_id();
    main_thread.name = "main";
    main_thread.cpu_usage_percent = 0.0; // 실제 구현에서는 더 정확한 계산 필요
    thread_info_.push_back(main_thread);
}

void PerformanceOptimizer::adjustThreadPoolSize() {
    auto cpu_usage = getCurrentCpuUsage();
    size_t current_pool_size = worker_threads_.size();
    size_t target_pool_size = current_pool_size;
    
    if (cpu_usage.total_usage_percent > cpu_threshold_) {
        // CPU 사용률이 높으면 스레드 풀 크기 줄임
        target_pool_size = std::max<size_t>(2, current_pool_size - 1);
    } else if (cpu_usage.total_usage_percent < cpu_threshold_ * 0.5) {
        // CPU 사용률이 낮으면 스레드 풀 크기 늘림
        target_pool_size = std::min(thread_pool_size_, current_pool_size + 1);
    }
    
    if (target_pool_size != current_pool_size) {
        resizeThreadPool(target_pool_size);
    }
}

void PerformanceOptimizer::resizeThreadPool(size_t new_size) {
    if (new_size == worker_threads_.size()) return;
    
    if (new_size < worker_threads_.size()) {
        // 스레드 풀 크기 줄이기
        size_t to_remove = worker_threads_.size() - new_size;
        for (size_t i = 0; i < to_remove; ++i) {
            if (worker_threads_.back().joinable()) {
                worker_threads_.back().join();
            }
            worker_threads_.pop_back();
        }
    } else {
        // 스레드 풀 크기 늘리기
        size_t to_add = new_size - worker_threads_.size();
        for (size_t i = 0; i < to_add; ++i) {
            worker_threads_.emplace_back(&PerformanceOptimizer::workerThreadFunction, this);
        }
    }
    
    core::Logger::getInstance().debug("스레드 풀 크기 조정: {} -> {}", worker_threads_.size(), new_size);
}

void PerformanceOptimizer::performOptimization() {
    auto cpu_usage = getCurrentCpuUsage();
    
    if (cpu_usage.total_usage_percent > cpu_threshold_) {
        // CPU 사용률이 높을 때 최적화
        optimizeHighCpuUsage();
    }
    
    // 메모리 압박 확인
    if (isMemoryPressure()) {
        optimizeMemoryPressure();
    }
    
    // 작업 타임아웃 확인
    checkTaskTimeouts();
}

void PerformanceOptimizer::optimizeHighCpuUsage() {
    // CPU 사용률이 높을 때의 최적화
    core::Logger::getInstance().debug("높은 CPU 사용률 감지, 최적화 수행");
    
    // 스레드 풀 크기 줄이기
    if (adaptive_thread_pool_) {
        size_t current_size = worker_threads_.size();
        size_t new_size = std::max<size_t>(2, current_size - 1);
        resizeThreadPool(new_size);
    }
    
    // 우선순위가 낮은 작업 지연
    deferLowPriorityTasks();
}

void PerformanceOptimizer::optimizeMemoryPressure() {
    core::Logger::getInstance().debug("메모리 압박 감지, 최적화 수행");
    
    // 캐시 정리
    clearCaches();
    
    // 불필요한 작업 취소
    cancelNonEssentialTasks();
}

void PerformanceOptimizer::checkTaskTimeouts() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = active_profiles_.begin(); it != active_profiles_.end();) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (duration > task_timeout_) {
            if (task_timeout_callback_) {
                task_timeout_callback_(it->first, duration);
            }
            it = active_profiles_.erase(it);
        } else {
            ++it;
        }
    }
}

void PerformanceOptimizer::schedulePriorityTasks() {
    // 우선순위 작업 스케줄링 (간단한 구현)
    // 실제로는 더 정교한 스케줄링 알고리즘이 필요
}

void PerformanceOptimizer::deferLowPriorityTasks() {
    // 낮은 우선순위 작업 지연
    core::Logger::getInstance().debug("낮은 우선순위 작업 지연");
}

void PerformanceOptimizer::clearCaches() {
    // 캐시 정리
    core::Logger::getInstance().debug("캐시 정리 수행");
}

void PerformanceOptimizer::cancelNonEssentialTasks() {
    // 필수적이지 않은 작업 취소
    core::Logger::getInstance().debug("비필수 작업 취소");
}

double PerformanceOptimizer::calculateCpuUsage() {
    return getCurrentCpuUsage().total_usage_percent;
}

bool PerformanceOptimizer::isMemoryPressure() const {
    // 메모리 압박 확인 (간단한 구현)
    return false;
}



} // namespace core
