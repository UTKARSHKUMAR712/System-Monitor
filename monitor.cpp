#include <windows.h>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

// Global tracking variables
volatile bool isRunning = true;
std::ofstream logFile;
HWND lastForegroundWindow = NULL;
std::string lastAppName = "";
std::chrono::system_clock::time_point lastStartTime;

// Handle console events
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT || signal == CTRL_SHUTDOWN_EVENT || signal == CTRL_LOGOFF_EVENT) {
        isRunning = false;
        return TRUE;
    }
    return FALSE;
}

std::string generateLogFileName() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "logs/log_";
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    ss << ".txt";
    return ss.str();
}

std::string formatTime(std::chrono::system_clock::time_point tp) {
    auto in_time_t = std::chrono::system_clock::to_time_t(tp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S"); 
    return ss.str();
}

// Get the EXE name of whatever window is active
std::string getProcessName(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return "Unknown";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return "Unknown";

    char processPath[MAX_PATH];
    DWORD size = MAX_PATH;
    std::string name = "Unknown";
    
    if (QueryFullProcessImageNameA(hProcess, 0, processPath, &size)) {
        std::string fullPath(processPath);
        // Extract just the .exe name from the full path
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            name = fullPath.substr(lastSlash + 1);
        } else {
            name = fullPath;
        }
    }
    CloseHandle(hProcess);
    return name;
}

void AddToStartup() {
    char exePath[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exePath, MAX_PATH)) return;

    char* appdata = getenv("APPDATA");
    if (appdata == NULL) return;

    std::string startupPath = std::string(appdata) + "\\Microsoft\\Windows\\Start Menu\\Programs\\Startup";
    std::string linkPath = startupPath + "\\SystemMonitor.lnk";

    // If the shortcut already exists, don't recreate it
    if (GetFileAttributesA(linkPath.c_str()) != INVALID_FILE_ATTRIBUTES) return;

    // Use a temporary VBScript to create the .lnk shortcut natively without COM linking requirements
    std::string vbsPath = startupPath + "\\createshortcut.vbs";
    std::ofstream vbs(vbsPath);
    vbs << "Set oWS = WScript.CreateObject(\"WScript.Shell\")\n";
    vbs << "sLinkFile = \"" << linkPath << "\"\n";
    vbs << "Set oLink = oWS.CreateShortcut(sLinkFile)\n";
    vbs << "oLink.TargetPath = \"" << exePath << "\"\n";
    
    std::string exeDir = exePath;
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        vbs << "oLink.WorkingDirectory = \"" << exeDir.substr(0, lastSlash) << "\"\n";
    }
    vbs << "oLink.Save\n";
    vbs.close();

    std::string cmd = "wscript.exe \"" + vbsPath + "\"";
    system(cmd.c_str());
    remove(vbsPath.c_str());
}

int main() {
    // Ensure working directory is the executable's directory so logs are saved locally on startup
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        std::string fullPath(exePath);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            std::string dir = fullPath.substr(0, lastSlash);
            SetCurrentDirectoryA(dir.c_str());
        }
    }

    // Automatically add this executable to the Windows Startup folder
    AddToStartup();

    // Prevent duplicate monitors
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "Local\\SystemMonitorMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    // Hide console window if one exists
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) ShowWindow(hwnd, SW_HIDE);
    
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    // Listen for Killswitch
    HANDLE hExitEvent = CreateEventA(NULL, FALSE, FALSE, "Local\\MonitorExitEvent");
    if (hExitEvent) ResetEvent(hExitEvent);
    
    CreateDirectoryA("logs", NULL);
    
    std::string logFileName = generateLogFileName();
    logFile.open(logFileName, std::ios::app);
    if (!logFile.is_open()) {
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }
    
    logFile << "AppName | StartTime | EndTime\n";
    logFile << "------------------------------------------------------\n";
    logFile.flush();

    // SUPER LIGHTWEIGHT LOOP: Only checks Foreground Window
    while (isRunning) {
        if (hExitEvent && WaitForSingleObject(hExitEvent, 0) == WAIT_OBJECT_0) {
            isRunning = false;
            break;
        }

        // Check which window the user is currently looking at
        HWND currentForeground = GetForegroundWindow();
        
        // If the user clicked/switched to a different window than the last one we checked
        if (currentForeground != NULL && currentForeground != lastForegroundWindow) {
            auto now = std::chrono::system_clock::now();
            
            // 1. Log the app that just lost focus (with its StartTime and EndTime)
            if (lastForegroundWindow != NULL && lastAppName != "Unknown" && lastAppName != "") {
                logFile << lastAppName << " | " 
                        << formatTime(lastStartTime) << " | " 
                        << formatTime(now) << "\n";
                logFile.flush();
            }
            
            // 2. Begin tracking the NEW app that just gained focus
            lastForegroundWindow = currentForeground;
            lastAppName = getProcessName(currentForeground);
            lastStartTime = now;
        }
        
        // Sleep for 0.5 seconds. Uses 0% CPU.
        Sleep(500); 
    }
    
    // On exit (killswitch or shutdown), log the final app the user was looking at
    if (lastForegroundWindow != NULL && lastAppName != "Unknown" && lastAppName != "") {
        auto now = std::chrono::system_clock::now();
        logFile << lastAppName << " | " 
                << formatTime(lastStartTime) << " | " 
                << formatTime(now) << "\n";
    }
    
    if (hExitEvent) CloseHandle(hExitEvent);
    if (hMutex) CloseHandle(hMutex);
    logFile.close();
    return 0;
}
