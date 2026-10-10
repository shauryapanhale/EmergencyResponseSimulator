#include "ers/services/EventLog.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>
#include "ers/common/Exceptions.h"

namespace ers {

void EventLog::append(double simTime, std::string entityType, int entityId, std::string eventType,
                      std::string details) {
    entries_.push_back(LogEntry{simTime, std::move(entityType), entityId, std::move(eventType),
                                std::move(details)});
}

std::vector<LogEntry> EventLog::filterByType(const std::string& eventType) const {
    std::vector<LogEntry> out;
    for (const auto& e : entries_)
        if (e.eventType == eventType) out.push_back(e);
    return out;
}

std::string EventLog::format(const LogEntry& e) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << "[t=" << e.simTime << "] " << e.entityType << " "
       << e.entityId << " " << e.eventType;
    if (!e.details.empty()) os << " - " << e.details;
    return os.str();
}

void EventLog::exportToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out) throw ErsException("Cannot open log file for writing: " + path);
    for (const auto& e : entries_) out << format(e) << '\n';
}

} // namespace ers