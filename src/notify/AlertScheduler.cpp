#include "AlertScheduler.h"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace notify {

// TimeWindow 구현
bool TimeWindow::isActive() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    int current_minutes = tm.tm_hour * 60 + tm.tm_min;
    int start_minutes = start_hour * 60 + start_minute;
    int end_minutes = end_hour * 60 + end_minute;
    
    if (start_minutes <= end_minutes) {
        // 같은 날 내의 시간대
        return current_minutes >= start_minutes && current_minutes <= end_minutes;
    } else {
        // 자정을 넘어가는 시간대 (예: 22:00 ~ 06:00)
        return current_minutes >= start_minutes || current_minutes <= end_minutes;
    }
}

std::string TimeWindow::toString() const {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << start_hour << ":"
       << std::setfill('0') << std::setw(2) << start_minute << " - "
       << std::setfill('0') << std::setw(2) << end_hour << ":"
       << std::setfill('0') << std::setw(2) << end_minute;
    return ss.str();
}

// AlertSchedule 구현
AlertSchedule::AlertSchedule() 
    : type(ScheduleType::ALWAYS)
    , enabled(true)
    , muted(false)
    , mute_duration(std::chrono::minutes(0))
    , priority(5) {
    
    time_window = {0, 0, 23, 59}; // 기본값: 24시간
    weekdays = {Weekday::MONDAY, Weekday::TUESDAY, Weekday::WEDNESDAY, 
                Weekday::THURSDAY, Weekday::FRIDAY, Weekday::SATURDAY, Weekday::SUNDAY};
}

bool AlertSchedule::isActive() const {
    if (!enabled) return false;
    if (isMuted()) return false;
    
    switch (type) {
        case ScheduleType::ALWAYS:
            return true;
            
        case ScheduleType::TIME_WINDOW:
            return time_window.isActive();
            
        case ScheduleType::WEEKDAY: {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auto tm = *std::localtime(&time_t);
            
            Weekday current_day = static_cast<Weekday>(tm.tm_wday);
            return std::find(weekdays.begin(), weekdays.end(), current_day) != weekdays.end();
        }
        
        case ScheduleType::CUSTOM:
            // 사용자 정의 규칙 평가는 AlertScheduler에서 처리
            return true;
            
        default:
            return false;
    }
}

bool AlertSchedule::isMuted() const {
    if (!muted) return false;
    
    auto now = std::chrono::system_clock::now();
    return now < mute_until;
}

void AlertSchedule::mute(std::chrono::minutes duration) {
    muted = true;
    mute_duration = duration;
    mute_until = std::chrono::system_clock::now() + duration;
}

void AlertSchedule::unmute() {
    muted = false;
    mute_until = std::chrono::system_clock::time_point();
}

std::string AlertSchedule::getStatus() const {
    if (!enabled) return "비활성화";
    if (isMuted()) return "음소거됨";
    if (isActive()) return "활성";
    return "비활성";
}

// AlertScheduler 구현
AlertScheduler::AlertScheduler() {
    createDefaultSchedules();
}

AlertScheduler::~AlertScheduler() = default;

void AlertScheduler::addSchedule(const AlertSchedule& schedule) {
    // 중복 ID 체크
    auto it = std::find_if(schedules_.begin(), schedules_.end(),
                          [&](const AlertSchedule& s) { return s.id == schedule.id; });
    if (it != schedules_.end()) {
        std::cerr << "스케줄 ID가 이미 존재합니다: " << schedule.id << std::endl;
        return;
    }
    
    schedules_.push_back(schedule);
}

void AlertScheduler::removeSchedule(const std::string& id) {
    schedules_.erase(
        std::remove_if(schedules_.begin(), schedules_.end(),
                      [&](const AlertSchedule& s) { return s.id == id; }),
        schedules_.end()
    );
}

void AlertScheduler::updateSchedule(const AlertSchedule& schedule) {
    auto it = std::find_if(schedules_.begin(), schedules_.end(),
                          [&](const AlertSchedule& s) { return s.id == schedule.id; });
    if (it != schedules_.end()) {
        *it = schedule;
    }
}

AlertSchedule* AlertScheduler::getSchedule(const std::string& id) {
    auto it = std::find_if(schedules_.begin(), schedules_.end(),
                          [&](const AlertSchedule& s) { return s.id == id; });
    return it != schedules_.end() ? &(*it) : nullptr;
}

std::vector<AlertSchedule> AlertScheduler::getAllSchedules() const {
    return schedules_;
}

void AlertScheduler::enableSchedule(const std::string& id) {
    auto schedule = getSchedule(id);
    if (schedule) {
        schedule->enabled = true;
    }
}

void AlertScheduler::disableSchedule(const std::string& id) {
    auto schedule = getSchedule(id);
    if (schedule) {
        schedule->enabled = false;
    }
}

void AlertScheduler::muteSchedule(const std::string& id, std::chrono::minutes duration) {
    auto schedule = getSchedule(id);
    if (schedule) {
        schedule->mute(duration);
    }
}

void AlertScheduler::unmuteSchedule(const std::string& id) {
    auto schedule = getSchedule(id);
    if (schedule) {
        schedule->unmute();
    }
}

void AlertScheduler::muteAll(std::chrono::minutes duration) {
    for (auto& schedule : schedules_) {
        schedule.mute(duration);
    }
}

void AlertScheduler::unmuteAll() {
    for (auto& schedule : schedules_) {
        schedule.unmute();
    }
}

bool AlertScheduler::shouldSendAlert(const std::string& schedule_id) const {
    auto it = std::find_if(schedules_.begin(), schedules_.end(),
                          [&](const AlertSchedule& s) { return s.id == schedule_id; });
    if (it == schedules_.end()) return false;
    
    const AlertSchedule& schedule = *it;
    if (!schedule.isActive()) return false;
    
    // 사용자 정의 규칙 평가
    if (schedule.type == ScheduleType::CUSTOM) {
        return evaluateCustomRules(schedule.custom_rules);
    }
    
    return true;
}

std::vector<std::string> AlertScheduler::getActiveSchedules() const {
    std::vector<std::string> active_schedules;
    
    for (const auto& schedule : schedules_) {
        if (schedule.isActive()) {
            active_schedules.push_back(schedule.id);
        }
    }
    
    return active_schedules;
}

void AlertScheduler::loadSchedules(const json& config) {
    schedules_.clear();
    
    if (config.contains("schedules") && config["schedules"].is_array()) {
        for (const auto& schedule_json : config["schedules"]) {
            AlertSchedule schedule;
            
            schedule.id = schedule_json.value("id", "");
            schedule.name = schedule_json.value("name", "");
            schedule.enabled = schedule_json.value("enabled", true);
            schedule.priority = schedule_json.value("priority", 5);
            
            // 타입 설정
            std::string type_str = schedule_json.value("type", "always");
            if (type_str == "always") schedule.type = ScheduleType::ALWAYS;
            else if (type_str == "time_window") schedule.type = ScheduleType::TIME_WINDOW;
            else if (type_str == "weekday") schedule.type = ScheduleType::WEEKDAY;
            else if (type_str == "custom") schedule.type = ScheduleType::CUSTOM;
            
            // 시간대 설정
            if (schedule_json.contains("time_window")) {
                auto& tw = schedule_json["time_window"];
                schedule.time_window.start_hour = tw.value("start_hour", 0);
                schedule.time_window.start_minute = tw.value("start_minute", 0);
                schedule.time_window.end_hour = tw.value("end_hour", 23);
                schedule.time_window.end_minute = tw.value("end_minute", 59);
            }
            
            // 요일 설정
            if (schedule_json.contains("weekdays") && schedule_json["weekdays"].is_array()) {
                schedule.weekdays.clear();
                for (const auto& day : schedule_json["weekdays"]) {
                    schedule.weekdays.push_back(static_cast<Weekday>(day.get<int>()));
                }
            }
            
            // 음소거 설정
            schedule.muted = schedule_json.value("muted", false);
            if (schedule_json.contains("mute_until")) {
                std::string mute_until_str = schedule_json["mute_until"];
                // ISO 8601 형식 파싱 (간단한 구현)
                // 실제로는 더 정교한 파싱 필요
            }
            
            // 사용자 정의 규칙
            if (schedule_json.contains("custom_rules")) {
                schedule.custom_rules = schedule_json["custom_rules"];
            }
            
            schedules_.push_back(schedule);
        }
    }
}

json AlertScheduler::saveSchedules() const {
    json config;
    json schedules_array = json::array();
    
    for (const auto& schedule : schedules_) {
        json schedule_json;
        schedule_json["id"] = schedule.id;
        schedule_json["name"] = schedule.name;
        schedule_json["enabled"] = schedule.enabled;
        schedule_json["priority"] = schedule.priority;
        
        // 타입 저장
        switch (schedule.type) {
            case ScheduleType::ALWAYS: schedule_json["type"] = "always"; break;
            case ScheduleType::TIME_WINDOW: schedule_json["type"] = "time_window"; break;
            case ScheduleType::WEEKDAY: schedule_json["type"] = "weekday"; break;
            case ScheduleType::CUSTOM: schedule_json["type"] = "custom"; break;
        }
        
        // 시간대 저장
        schedule_json["time_window"] = {
            {"start_hour", schedule.time_window.start_hour},
            {"start_minute", schedule.time_window.start_minute},
            {"end_hour", schedule.time_window.end_hour},
            {"end_minute", schedule.time_window.end_minute}
        };
        
        // 요일 저장
        schedule_json["weekdays"] = json::array();
        for (const auto& day : schedule.weekdays) {
            schedule_json["weekdays"].push_back(static_cast<int>(day));
        }
        
        // 음소거 설정 저장
        schedule_json["muted"] = schedule.muted;
        if (schedule.muted) {
            auto time_t = std::chrono::system_clock::to_time_t(schedule.mute_until);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
            schedule_json["mute_until"] = ss.str();
        }
        
        // 사용자 정의 규칙 저장
        if (!schedule.custom_rules.is_null()) {
            schedule_json["custom_rules"] = schedule.custom_rules;
        }
        
        schedules_array.push_back(schedule_json);
    }
    
    config["schedules"] = schedules_array;
    return config;
}

void AlertScheduler::createDefaultSchedules() {
    // 기본 스케줄: 24시간 활성
    AlertSchedule always_schedule;
    always_schedule.id = "always";
    always_schedule.name = "24시간 알림";
    always_schedule.type = ScheduleType::ALWAYS;
    always_schedule.priority = 5;
    schedules_.push_back(always_schedule);
    
    // 업무시간 스케줄: 평일 9시-18시
    AlertSchedule work_schedule;
    work_schedule.id = "work_hours";
    work_schedule.name = "업무시간 알림";
    work_schedule.type = ScheduleType::TIME_WINDOW;
    work_schedule.time_window = {9, 0, 18, 0};
    work_schedule.weekdays = {Weekday::MONDAY, Weekday::TUESDAY, Weekday::WEDNESDAY, 
                             Weekday::THURSDAY, Weekday::FRIDAY};
    work_schedule.priority = 8;
    schedules_.push_back(work_schedule);
    
    // 야간 스케줄: 22시-06시
    AlertSchedule night_schedule;
    night_schedule.id = "night_hours";
    night_schedule.name = "야간 알림";
    night_schedule.type = ScheduleType::TIME_WINDOW;
    night_schedule.time_window = {22, 0, 6, 0};
    night_schedule.priority = 10; // 야간 알림은 높은 우선순위
    schedules_.push_back(night_schedule);
}

bool AlertScheduler::isTimeInWindow(const TimeWindow& window) const {
    return window.isActive();
}

bool AlertScheduler::isWeekdayActive(const std::vector<Weekday>& weekdays) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    Weekday current_day = static_cast<Weekday>(tm.tm_wday);
    return std::find(weekdays.begin(), weekdays.end(), current_day) != weekdays.end();
}

bool AlertScheduler::evaluateCustomRules(const json& rules) const {
    // 사용자 정의 규칙 평가 (기본 구현)
    // 실제로는 더 복잡한 규칙 엔진 필요
    if (rules.is_null()) return true;
    
    // 예시: CPU 사용률이 90% 이상일 때만 알림
    if (rules.contains("cpu_threshold")) {
        double threshold = rules["cpu_threshold"];
        // 실제 CPU 사용률과 비교하는 로직 필요
        // 여기서는 기본값 반환
        return true;
    }
    
    return true;
}

} // namespace notify
