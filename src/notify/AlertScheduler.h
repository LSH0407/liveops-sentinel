#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace notify {

enum class ScheduleType {
    ALWAYS,         // 항상 알림
    TIME_WINDOW,    // 시간대별 알림
    WEEKDAY,        // 요일별 알림
    CUSTOM          // 사용자 정의 스케줄
};

enum class Weekday {
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 0
};

struct TimeWindow {
    int start_hour;
    int start_minute;
    int end_hour;
    int end_minute;
    
    bool isActive() const;
    std::string toString() const;
};

struct AlertSchedule {
    std::string id;
    std::string name;
    ScheduleType type;
    bool enabled;
    
    // 시간대별 설정
    TimeWindow time_window;
    
    // 요일별 설정
    std::vector<Weekday> weekdays;
    
    // 음소거 설정
    bool muted;
    std::chrono::minutes mute_duration;
    std::chrono::system_clock::time_point mute_until;
    
    // 우선순위 설정
    int priority; // 1-10, 10이 가장 높음
    
    // 사용자 정의 스케줄 (JSON 형태)
    json custom_rules;
    
    AlertSchedule();
    bool isActive() const;
    bool isMuted() const;
    void mute(std::chrono::minutes duration);
    void unmute();
    std::string getStatus() const;
};

class AlertScheduler {
public:
    AlertScheduler();
    ~AlertScheduler();
    
    // 스케줄 관리
    void addSchedule(const AlertSchedule& schedule);
    void removeSchedule(const std::string& id);
    void updateSchedule(const AlertSchedule& schedule);
    AlertSchedule* getSchedule(const std::string& id);
    std::vector<AlertSchedule> getAllSchedules() const;
    
    // 스케줄 활성화/비활성화
    void enableSchedule(const std::string& id);
    void disableSchedule(const std::string& id);
    
    // 음소거 관리
    void muteSchedule(const std::string& id, std::chrono::minutes duration);
    void unmuteSchedule(const std::string& id);
    void muteAll(std::chrono::minutes duration);
    void unmuteAll();
    
    // 알림 필터링
    bool shouldSendAlert(const std::string& schedule_id) const;
    std::vector<std::string> getActiveSchedules() const;
    
    // 설정 관리
    void loadSchedules(const json& config);
    json saveSchedules() const;
    
    // 기본 스케줄 생성
    void createDefaultSchedules();
    
private:
    std::vector<AlertSchedule> schedules_;
    
    bool isTimeInWindow(const TimeWindow& window) const;
    bool isWeekdayActive(const std::vector<Weekday>& weekdays) const;
    bool evaluateCustomRules(const json& rules) const;
};

} // namespace notify
