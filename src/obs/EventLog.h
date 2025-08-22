#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace obs {

struct Event {
    std::chrono::system_clock::time_point timestamp;
    std::string type;
    nlohmann::json payload;
    
    Event() = default;
    Event(const std::string& eventType, const nlohmann::json& eventPayload)
        : timestamp(std::chrono::system_clock::now())
        , type(eventType)
        , payload(eventPayload) {}
};

class EventLog {
public:
    explicit EventLog(size_t maxEvents = 500);
    
    // Thread-safe operations
    void push(const Event& event);
    std::vector<Event> getRecentEvents(size_t count = 200) const;
    std::vector<Event> getEventsByType(const std::string& type, size_t maxCount = 100) const;
    void clear();
    size_t size() const;
    
    // File operations
    bool saveToJson(const std::string& path) const;
    bool loadFromJson(const std::string& path);
    
    // Filtering
    std::vector<Event> searchEvents(const std::string& searchTerm, size_t maxCount = 100) const;

private:
    mutable std::mutex mutex_;
    std::vector<Event> events_;
    size_t maxEvents_;
    size_t currentIndex_;
    bool bufferFull_;
};

} // namespace obs
