#pragma once
#include <string>
#include <vector>
#include <map>

struct Session {
    int startSec;
    int endSec;
};

struct AppData {
    std::string name;
    std::vector<Session> sessions;
    int totalUsageSec;
};

class LogParser {
public:
    // Scans the logs folder and returns a list of unique dates (YYYY-MM-DD)
    static std::vector<std::string> GetAvailableDates(const std::string& logsDir);

    // Reads ALL log files for a specific date, merges the sessions, and returns the sorted AppData
    static std::vector<AppData> ProcessLogsForDate(const std::string& logsDir, const std::string& date, int gapLimitSec, bool hideMicroSessions);

    // Formatting utilities
    static std::string FormatAMPM(int totalSeconds);
    static std::string FormatDuration(int totalSeconds);

private:
    static std::string Trim(const std::string& str);
    static int TimeToSeconds(const std::string& t);
    static std::vector<Session> MergeSessions(std::vector<Session>& sessions, int gapLimitSec);
};
