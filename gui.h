#pragma once
#include <string>
#include <vector>
#include "log_parser.h"

struct TimelineEvent {
    int startSec;
    int endSec;
    std::string appName;
};

class GUI {
public:
    GUI();
    void Render();

private:
    void RefreshLogs();
    void RenderAppsView();
    void RenderTimelineView();

    std::vector<std::string> availableDates;
    int selectedDateIdx = 0;
    int mergeGapMin = 5;
    bool hideMicroSessions = false;
    bool tileView = false;
    
    std::vector<AppData> processedApps;
    std::vector<TimelineEvent> timelineEvents;
    int totalDayUsageSec = 0;
    bool needsRefresh = true;
};
