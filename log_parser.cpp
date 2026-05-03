#include "log_parser.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

std::string LogParser::Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

int LogParser::TimeToSeconds(const std::string& t) {
    int h = 0, m = 0, s = 0;
    char sep1, sep2;
    std::stringstream ss(t);
    if (ss >> h >> sep1 >> m >> sep2 >> s && sep1 == ':' && sep2 == ':') {
        return h * 3600 + m * 60 + s;
    }
    return -1;
}

std::string LogParser::FormatAMPM(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    
    std::string period = (h >= 12) ? "PM" : "AM";
    if (h == 0) h = 12;
    else if (h > 12) h -= 12;
    
    std::stringstream ss;
    ss << h << ":" << std::setw(2) << std::setfill('0') << m << ":" << std::setw(2) << std::setfill('0') << s << " " << period;
    return ss.str();
}

std::string LogParser::FormatDuration(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    
    std::stringstream ss;
    if (h > 0) ss << h << "h";
    if (m > 0 || (h > 0 && s > 0)) ss << m << "m"; 
    if (s > 0 || (h == 0 && m == 0)) ss << s << "s"; 
    return ss.str();
}

std::vector<Session> LogParser::MergeSessions(std::vector<Session>& sessions, int gapLimitSec) {
    if (sessions.empty()) return {};
    
    std::sort(sessions.begin(), sessions.end(), [](const Session& a, const Session& b) {
        return a.startSec < b.startSec;
    });
    
    std::vector<Session> merged;
    merged.push_back(sessions[0]);
    
    for (size_t i = 1; i < sessions.size(); ++i) {
        Session& last = merged.back();
        if (sessions[i].startSec - last.endSec <= gapLimitSec) {
            last.endSec = std::max(last.endSec, sessions[i].endSec);
        } else {
            merged.push_back(sessions[i]);
        }
    }
    return merged;
}

std::vector<std::string> LogParser::GetAvailableDates(const std::string& logsDir) {
    std::set<std::string> datesSet; // Use set to auto-sort and unique
    if (fs::exists(logsDir) && fs::is_directory(logsDir)) {
        for (const auto& entry : fs::directory_iterator(logsDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // Format: log_YYYY-MM-DD_HH-MM-SS.txt
                if (filename.find("log_") == 0 && filename.length() >= 14) {
                    std::string date = filename.substr(4, 10);
                    datesSet.insert(date);
                }
            }
        }
    }
    // Return sorted (descending, latest first)
    std::vector<std::string> dates(datesSet.begin(), datesSet.end());
    std::sort(dates.rbegin(), dates.rend());
    return dates;
}

std::vector<AppData> LogParser::ProcessLogsForDate(const std::string& logsDir, const std::string& date, int gapLimitSec, bool hideMicroSessions) {
    std::map<std::string, std::vector<Session>> appSessions;
    
    if (fs::exists(logsDir) && fs::is_directory(logsDir)) {
        for (const auto& entry : fs::directory_iterator(logsDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                // Find all logs matching "log_YYYY-MM-DD"
                if (filename.find("log_" + date) == 0) {
                    std::ifstream file(entry.path().string());
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.empty() || line.find("AppName") != std::string::npos || line.find("---") != std::string::npos) {
                            continue;
                        }
                        
                        std::stringstream ss(line);
                        std::string appName, startStr, endStr;
                        if (std::getline(ss, appName, '|') && std::getline(ss, startStr, '|') && std::getline(ss, endStr, '|')) {
                            appName = Trim(appName);
                            startStr = Trim(startStr);
                            endStr = Trim(endStr);
                            
                            int startSec = TimeToSeconds(startStr);
                            int endSec = TimeToSeconds(endStr);
                            
                            if (startSec != -1 && endSec != -1) {
                                if (endSec < startSec) endSec += 86400; // Midnight wrap
                                
                                int duration = endSec - startSec;
                                if (hideMicroSessions && duration < 3) continue; // Skip micro sessions
                                
                                appSessions[appName].push_back({startSec, endSec});
                            }
                        }
                    }
                }
            }
        }
    }
    
    std::vector<AppData> processedApps;
    for (auto& pair : appSessions) {
        AppData data;
        data.name = pair.first;
        data.sessions = MergeSessions(pair.second, gapLimitSec);
        
        data.totalUsageSec = 0;
        for (const auto& s : data.sessions) {
            data.totalUsageSec += (s.endSec - s.startSec);
        }
        
        if (data.totalUsageSec > 0) {
            processedApps.push_back(data);
        }
    }
    
    // Sort highest usage first
    std::sort(processedApps.begin(), processedApps.end(), [](const AppData& a, const AppData& b) {
        return a.totalUsageSec > b.totalUsageSec;
    });
    
    return processedApps;
}
