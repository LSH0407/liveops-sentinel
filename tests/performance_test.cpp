#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <memory>

#include "../src/core/PerformanceManager.h"
#include "../src/core/Logger.h"

using namespace core;

void simulateWorkload() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sleep_dist(10, 100);
    std::uniform_int_distribution<> work_dist(1, 10);
    
    for (int i = 0; i < 100; ++i) {
        // CPU 집약적 작업 시뮬레이션
        auto start = std::chrono::high_resolution_clock::now();
        volatile int result = 0;
        for (int j = 0; j < work_dist(gen) * 1000000; ++j) {
            result += j * j;
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        // 짧은 휴식
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dist(gen)));
        
        if (i % 10 == 0) {
            std::cout << "작업 " << i << " 완료, 결과: " << result << std::endl;
        }
    }
}

void testMemoryOperations() {
    auto& pm = PerformanceManager::getInstance();
    auto& mm = pm.getMemoryManager();
    
    std::cout << "\n=== 메모리 관리 테스트 ===" << std::endl;
    
    // 캐시에 데이터 추가
    for (int i = 0; i < 50; ++i) {
        std::string key = "test_key_" + std::to_string(i);
        std::string data(1024, 'A' + (i % 26)); // 1KB 데이터
        mm.addToCache(key, data);
    }
    
    std::cout << "캐시 크기: " << mm.getCacheSize() / 1024 << " KB" << std::endl;
    std::cout << "캐시 히트율: " << mm.getCacheHitRate() << "%" << std::endl;
    
    // 캐시에서 데이터 검색
    for (int i = 0; i < 20; ++i) {
        std::string key = "test_key_" + std::to_string(i);
        auto data = mm.getFromCache(key);
        if (!data.empty()) {
            std::cout << "캐시 히트: " << key << std::endl;
        }
    }
    
    std::cout << "최종 캐시 히트율: " << mm.getCacheHitRate() << "%" << std::endl;
}

void testPerformanceProfiles() {
    auto& pm = PerformanceManager::getInstance();
    
    std::cout << "\n=== 성능 프로파일 테스트 ===" << std::endl;
    
    // 균형 모드
    pm.setPerformanceProfile("balanced");
    std::cout << "현재 프로파일: " << pm.getCurrentProfile() << std::endl;
    std::cout << pm.getOptimizationStatus() << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 성능 우선 모드
    pm.setPerformanceProfile("performance");
    std::cout << "현재 프로파일: " << pm.getCurrentProfile() << std::endl;
    std::cout << pm.getOptimizationStatus() << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 보수적 모드
    pm.setPerformanceProfile("conservative");
    std::cout << "현재 프로파일: " << pm.getCurrentProfile() << std::endl;
    std::cout << pm.getOptimizationStatus() << std::endl;
}

void testThreadPool() {
    auto& pm = PerformanceManager::getInstance();
    auto& po = pm.getPerformanceOptimizer();
    
    std::cout << "\n=== 스레드 풀 테스트 ===" << std::endl;
    
    std::vector<std::future<void>> futures;
    
    // 여러 작업을 스레드 풀에 제출
    for (int i = 0; i < 20; ++i) {
        auto future = po.submitTask("test_task_" + std::to_string(i), [i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 + i * 10));
            std::cout << "작업 " << i << " 완료" << std::endl;
        });
        futures.push_back(std::move(future));
    }
    
    // 모든 작업 완료 대기
    for (auto& future : futures) {
        future.wait();
    }
    
    std::cout << "모든 작업 완료" << std::endl;
    
    // 작업 통계 출력
    auto stats = po.getStats();
    std::cout << "총 실행된 작업: " << stats.total_tasks_executed << std::endl;
    std::cout << "평균 작업 시간: " << 
        std::chrono::duration_cast<std::chrono::milliseconds>(stats.average_task_time).count() 
        << " ms" << std::endl;
}

int main() {
    try {
        // 로거 초기화
        Logger::getInstance().setLevel(LogLevel::INFO);
        
        std::cout << "=== LiveOps Sentinel 성능 최적화 테스트 ===" << std::endl;
        
        // 성능 관리자 초기화
        auto& pm = PerformanceManager::getInstance();
        pm.initialize();
        pm.startMonitoring();
        
        std::cout << "성능 관리자 초기화 완료" << std::endl;
        
        // 메모리 관리 테스트
        testMemoryOperations();
        
        // 성능 프로파일 테스트
        testPerformanceProfiles();
        
        // 스레드 풀 테스트
        testThreadPool();
        
        // 워크로드 시뮬레이션
        std::cout << "\n=== 워크로드 시뮬레이션 ===" << std::endl;
        simulateWorkload();
        
        // 최종 성능 보고서
        std::cout << "\n=== 최종 성능 보고서 ===" << std::endl;
        std::cout << pm.getOptimizationStatus() << std::endl;
        
        // 정리
        pm.stopMonitoring();
        pm.shutdown();
        
        std::cout << "\n테스트 완료" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "오류 발생: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
