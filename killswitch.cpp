#include <windows.h>
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "Attempting to shut down System Monitor gracefully..." << std::endl;
    
    // Open the named event that monitor.exe is listening to
    HANDLE hExitEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Local\\MonitorExitEvent");
    
    if (hExitEvent) {
        // Signal the event. monitor.exe will catch this, log the remaining apps, and exit.
        SetEvent(hExitEvent);
        CloseHandle(hExitEvent);
        
        std::cout << "\nSuccess: Sent exit signal to Monitor." << std::endl;
        std::cout << "The Monitor will now log the final times and safely exit." << std::endl;
    } else {
        std::cout << "\nEvent not found. Monitor might not be running or requires force kill." << std::endl;
        std::cout << "Force killing..." << std::endl;
        // Fallback in case the event fails or monitor is frozen
        std::system("taskkill /IM monitor.exe /F");
    }
    
    // Pause for 3 seconds so you can read the output
    Sleep(3000);
    return 0;
}
