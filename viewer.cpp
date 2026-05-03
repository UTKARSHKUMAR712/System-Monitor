#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>

struct Session {
    int startSec;
    int endSec;
};

struct AppData {
    std::string name;
    std::vector<Session> sessions;
    int totalUsageSec;
};

// Trim whitespace from string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

// Convert "HH:MM:SS" to total seconds since midnight
int timeToSeconds(const std::string& t) {
    int h = 0, m = 0, s = 0;
    char sep1, sep2;
    std::stringstream ss(t);
    if (ss >> h >> sep1 >> m >> sep2 >> s && sep1 == ':' && sep2 == ':') {
        return h * 3600 + m * 60 + s;
    }
    return -1;
}

// Format seconds to "12:MM:SS PM"
std::string formatAMPM(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    
    std::string period = (h >= 12) ? "PM" : "AM";
    if (h == 0) {
        h = 12;
    } else if (h > 12) {
        h -= 12;
    }
    
    std::stringstream ss;
    ss << h << ":" 
       << std::setw(2) << std::setfill('0') << m << ":" 
       << std::setw(2) << std::setfill('0') << s << " " 
       << period;
    return ss.str();
}

// Format duration to XhYmZs
std::string formatDuration(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    
    std::stringstream ss;
    if (h > 0) ss << h << "h";
    if (m > 0 || (h > 0 && s > 0)) ss << m << "m"; 
    if (s > 0 || (h == 0 && m == 0)) ss << s << "s"; 
    return ss.str();
}

// Merge sessions based on gap limit
std::vector<Session> mergeSessions(std::vector<Session>& sessions, int gapLimit) {
    if (sessions.empty()) return {};
    
    // Sort by start time chronologically
    std::sort(sessions.begin(), sessions.end(), [](const Session& a, const Session& b) {
        return a.startSec < b.startSec;
    });
    
    std::vector<Session> merged;
    merged.push_back(sessions[0]);
    
    for (size_t i = 1; i < sessions.size(); ++i) {
        Session& last = merged.back();
        
        // If the gap between previous end and next start <= gapLimit
        if (sessions[i].startSec - last.endSec <= gapLimit) {
            // Merge: new end is the max of both ends
            last.endSec = std::max(last.endSec, sessions[i].endSec);
        } else {
            // No merge, push as new session block
            merged.push_back(sessions[i]);
        }
    }
    return merged;
}

int main(int argc, char* argv[]) {
    std::string filename;
    int gapLimit = 300; // default 5 minutes
    
    // Support Command Line Arguments or interactive Prompt
    if (argc >= 2) {
        filename = argv[1];
        if (argc >= 3) {
            try { gapLimit = std::stoi(argv[2]) * 60; } catch (...) { gapLimit = 300; }
        }
    } else {
        std::string folderPath = "logs";
        std::cout << "Enter logs folder path (press Enter to use default 'logs'): ";
        std::string inputFolder;
        std::getline(std::cin, inputFolder);
        
        if (!inputFolder.empty()) {
            // Strip quotes if they dragged and dropped a folder
            if (inputFolder.front() == '"') inputFolder.erase(0, 1);
            if (inputFolder.back() == '"') inputFolder.pop_back();
            folderPath = inputFolder;
        }
        
        std::vector<std::string> logFiles;
        try {
            if (std::filesystem::exists(folderPath) && std::filesystem::is_directory(folderPath)) {
                for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                    if (entry.is_regular_file()) {
                        std::string pathStr = entry.path().string();
                        if (pathStr.find(".txt") != std::string::npos) {
                            logFiles.push_back(pathStr);
                        }
                    }
                }
            } else {
                std::cout << "Folder does not exist: " << folderPath << "\n";
                std::cout << "Press Enter to exit...";
                std::string dummy; std::getline(std::cin, dummy);
                return 1;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error accessing folder: " << e.what() << "\n";
            std::cout << "Press Enter to exit...";
            std::string dummy; std::getline(std::cin, dummy);
            return 1;
        }
        
        if (logFiles.empty()) {
            std::cout << "No log files found in folder: " << folderPath << "\n";
            std::cout << "Press Enter to exit...";
            std::string dummy; std::getline(std::cin, dummy);
            return 0;
        }
        
        // Sort descending so the latest is on top (based on filename timestamp)
        std::sort(logFiles.rbegin(), logFiles.rend());
        
        std::cout << "\nAvailable Log Files:\n";
        for (size_t i = 0; i < logFiles.size(); ++i) {
            std::filesystem::path p(logFiles[i]);
            std::cout << "[" << i + 1 << "] " << p.filename().string() << "\n";
        }
        
        std::cout << "\nEnter the number of the log file to view (1-" << logFiles.size() << "): ";
        std::string choiceStr;
        std::getline(std::cin, choiceStr);
        
        int choice = 0;
        try { choice = std::stoi(choiceStr); } catch (...) {}
        
        if (choice < 1 || choice > (int)logFiles.size()) {
            std::cout << "Invalid choice. Exiting.\n";
            std::cout << "Press Enter to exit...";
            std::string dummy; std::getline(std::cin, dummy);
            return 1;
        }
        
        filename = logFiles[choice - 1];
        
        std::cout << "\nEnter merge gap in minutes (press Enter for default 5 mins): ";
        std::string gapStr;
        std::getline(std::cin, gapStr);
        if (!gapStr.empty()) {
            try {
                gapLimit = std::stoi(gapStr) * 60;
            } catch (...) {
                std::cout << "Invalid gap. Using default 5 minutes.\n";
                gapLimit = 300;
            }
        }
    }
    
    // Strip quotes from filename just in case it was passed via CLI with quotes
    if (!filename.empty()) {
        if (filename.front() == '"') filename.erase(0, 1);
        if (filename.back() == '"') filename.pop_back();
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << "\n";
        std::cout << "Press Enter to exit...";
        std::string dummy; std::getline(std::cin, dummy);
        return 1;
    }
    
    std::map<std::string, std::vector<Session>> appSessions;
    std::string line;
    
    // Parse log file
    while (std::getline(file, line)) {
        // Skip headers and empty lines
        if (line.empty() || line.find("AppName") != std::string::npos || line.find("---") != std::string::npos) {
            continue; 
        }
        
        std::stringstream ss(line);
        std::string appName, startStr, endStr;
        
        // Parse "AppName | StartTime | EndTime"
        if (std::getline(ss, appName, '|') && 
            std::getline(ss, startStr, '|') && 
            std::getline(ss, endStr, '|')) {
            
            appName = trim(appName);
            startStr = trim(startStr);
            endStr = trim(endStr);
            
            int startSec = timeToSeconds(startStr);
            int endSec = timeToSeconds(endStr);
            
            if (startSec != -1 && endSec != -1) {
                // Handle midnight wrap-around (e.g., started at 23:50, ended at 00:10)
                if (endSec < startSec) {
                    endSec += 86400; // Add 24 hours
                }
                appSessions[appName].push_back({startSec, endSec});
            }
        }
    }
    
    file.close();
    
    // Process and merge sessions
    std::vector<AppData> processedApps;
    for (auto& pair : appSessions) {
        AppData data;
        data.name = pair.first;
        data.sessions = mergeSessions(pair.second, gapLimit);
        
        // Calculate Total Usage for the app
        data.totalUsageSec = 0;
        for (const auto& s : data.sessions) {
            data.totalUsageSec += (s.endSec - s.startSec);
        }
        
        if (data.totalUsageSec > 0) {
            processedApps.push_back(data);
        }
    }
    
    // Sort apps by highest total usage
    std::sort(processedApps.begin(), processedApps.end(), [](const AppData& a, const AppData& b) {
        return a.totalUsageSec > b.totalUsageSec;
    });
    
    // Print the output
    std::cout << "\n-----------------------------------\n";
    std::cout << "       System Usage Viewer         \n";
    std::cout << "    (Sorted by Highest Usage)    \n";
    std::cout << "-----------------------------------\n\n";
    
    if (processedApps.empty()) {
        std::cout << "No valid session data found in the log.\n";
    }
    
    for (const auto& app : processedApps) {
        std::cout << "[ " << app.name << " ]\n\n";
        
        int sessionNum = 1;
        for (const auto& s : app.sessions) {
            std::cout << "Session " << sessionNum++ << ":\n";
            
            // Format back into 24-hour cycle if it wrapped around midnight
            int startDisplay = s.startSec % 86400;
            int endDisplay = s.endSec % 86400;
            
            std::cout << "Start: " << formatAMPM(startDisplay) << "\n";
            std::cout << "End:   " << formatAMPM(endDisplay) << "\n";
            std::cout << "Usage: " << formatDuration(s.endSec - s.startSec) << "\n\n";
        }
        
        std::cout << "-----------------------------------\n";
        std::cout << "Total Usage: " << formatDuration(app.totalUsageSec) << "\n";
        std::cout << "-----------------------------------\n\n";
    }
    
    // Pause before exiting so the user can read the output if double-clicked
    std::cout << "Press Enter to exit...";
    std::string dummy;
    std::getline(std::cin, dummy);
    
    return 0;
}
