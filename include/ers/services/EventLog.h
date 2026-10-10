#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace ers {

struct LogEntry {
    double simTime;
    std::string entityType;  // Incident, Vehicle, Dispatch, Road, Simulation
    int entityId;
    std::string eventType;
    std::string details;
};

class EventLog {
public:
    void append(double simTime, std::string entityType, int entityId, std::string eventType,
                std::string details = "");
    const std::vector<LogEntry>& entries() const { return entries_; }
    std::size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }
    std::vector<LogEntry> filterByType(const std::string& eventType) const;

    static std::string format(const LogEntry& e);
    // File handling: plain-text export using C++ streams
    void exportToFile(const std::string& path) const;

private:
    std::vector<LogEntry> entries_;
};

} // namespace ers