#include "EventLog.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace obs {

EventLog::EventLog(size_t maxEvents)
    : maxEvents_(maxEvents)
    , currentIndex_(0)
    , bufferFull_(false) {
    events_.reserve(maxEvents);
}

void EventLog::push(const Event& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (bufferFull_) {
        // Overwrite oldest event
        events_[currentIndex_] = event;
    } else {
        // Add new event
        events_.push_back(event);
    }
    
    currentIndex_ = (currentIndex_ + 1) % maxEvents_;
    if (currentIndex_ == 0) {
        bufferFull_ = true;
    }
}

std::vector<Event> EventLog::getRecentEvents(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Event> result;
    if (events_.empty()) {
        return result;
    }
    
    count = std::min(count, events_.size());
    result.reserve(count);
    
    if (bufferFull_) {
        // Circular buffer is full, get most recent events
        size_t start = (currentIndex_ - count + maxEvents_) % maxEvents_;
        for (size_t i = 0; i < count; ++i) {
            size_t index = (start + i) % maxEvents_;
            result.push_back(events_[index]);
        }
    } else {
        // Buffer not full, get from end
        size_t start = events_.size() - count;
        for (size_t i = start; i < events_.size(); ++i) {
            result.push_back(events_[i]);
        }
    }
    
    return result;
}

std::vector<Event> EventLog::getEventsByType(const std::string& type, size_t maxCount) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Event> result;
    result.reserve(std::min(maxCount, events_.size()));
    
    if (bufferFull_) {
        // Search through circular buffer
        for (size_t i = 0; i < maxEvents_ && result.size() < maxCount; ++i) {
            size_t index = (currentIndex_ - 1 - i + maxEvents_) % maxEvents_;
            if (events_[index].type == type) {
                result.push_back(events_[index]);
            }
        }
    } else {
        // Search through linear buffer
        for (auto it = events_.rbegin(); it != events_.rend() && result.size() < maxCount; ++it) {
            if (it->type == type) {
                result.push_back(*it);
            }
        }
    }
    
    return result;
}

void EventLog::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
    currentIndex_ = 0;
    bufferFull_ = false;
}

size_t EventLog::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bufferFull_ ? maxEvents_ : events_.size();
}

bool EventLog::saveToJson(const std::string& path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        nlohmann::json j;
        j["metadata"] = {
            {"totalEvents", events_.size()},
            {"maxEvents", maxEvents_},
            {"bufferFull", bufferFull_},
            {"currentIndex", currentIndex_},
            {"exportTime", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        
        nlohmann::json eventsArray = nlohmann::json::array();
        for (const auto& event : events_) {
            nlohmann::json eventJson;
            eventJson["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                event.timestamp.time_since_epoch()).count();
            eventJson["type"] = event.type;
            eventJson["payload"] = event.payload;
            eventsArray.push_back(eventJson);
        }
        j["events"] = eventsArray;
        
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        file << j.dump(2);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool EventLog::loadFromJson(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
        
        if (j.contains("events") && j["events"].is_array()) {
            for (const auto& eventJson : j["events"]) {
                Event event;
                event.type = eventJson["type"].get<std::string>();
                event.payload = eventJson["payload"];
                
                if (eventJson.contains("timestamp")) {
                    auto timestampMs = eventJson["timestamp"].get<int64_t>();
                    event.timestamp = std::chrono::system_clock::time_point(
                        std::chrono::milliseconds(timestampMs));
                } else {
                    event.timestamp = std::chrono::system_clock::now();
                }
                
                events_.push_back(event);
            }
        }
        
        currentIndex_ = events_.size() % maxEvents_;
        bufferFull_ = events_.size() >= maxEvents_;
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<Event> EventLog::searchEvents(const std::string& searchTerm, size_t maxCount) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Event> result;
    result.reserve(std::min(maxCount, events_.size()));
    
    std::string lowerSearchTerm = searchTerm;
    std::transform(lowerSearchTerm.begin(), lowerSearchTerm.end(), 
                   lowerSearchTerm.begin(), ::tolower);
    
    if (bufferFull_) {
        // Search through circular buffer
        for (size_t i = 0; i < maxEvents_ && result.size() < maxCount; ++i) {
            size_t index = (currentIndex_ - 1 - i + maxEvents_) % maxEvents_;
            const auto& event = events_[index];
            
            std::string lowerType = event.type;
            std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
            
            if (lowerType.find(lowerSearchTerm) != std::string::npos) {
                result.push_back(event);
            }
        }
    } else {
        // Search through linear buffer
        for (auto it = events_.rbegin(); it != events_.rend() && result.size() < maxCount; ++it) {
            std::string lowerType = it->type;
            std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);
            
            if (lowerType.find(lowerSearchTerm) != std::string::npos) {
                result.push_back(*it);
            }
        }
    }
    
    return result;
}

} // namespace obs
