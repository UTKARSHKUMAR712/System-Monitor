#include "gui.h"
#include "imgui/imgui.h"
#include <algorithm>

GUI::GUI() {
    availableDates = LogParser::GetAvailableDates("logs");
}

void GUI::RefreshLogs() {
    if (availableDates.empty()) return;
    
    std::string date = availableDates[selectedDateIdx];
    processedApps = LogParser::ProcessLogsForDate("logs", date, mergeGapMin * 60, hideMicroSessions);
    
    totalDayUsageSec = 0;
    timelineEvents.clear();
    
    for (const auto& app : processedApps) {
        totalDayUsageSec += app.totalUsageSec;
        for (const auto& session : app.sessions) {
            TimelineEvent ev;
            ev.startSec = session.startSec;
            ev.endSec = session.endSec;
            ev.appName = app.name;
            timelineEvents.push_back(ev);
        }
    }
    
    // Sort events chronologically
    std::sort(timelineEvents.begin(), timelineEvents.end(), [](const TimelineEvent& a, const TimelineEvent& b) {
        return a.startSec < b.startSec;
    });
    
    needsRefresh = false;
}

void GUI::Render() {
    if (needsRefresh) {
        RefreshLogs();
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Daily Tracker Viewer", nullptr, flags);
    
    ImGui::Text("Daily System Usage Tracker");
    ImGui::Separator();
    
    if (availableDates.empty()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No logs found in 'logs' folder.");
        ImGui::End();
        return;
    }
    
    // Date Selector
    const char* previewValue = availableDates[selectedDateIdx].c_str();
    if (ImGui::BeginCombo("Select Date", previewValue)) {
        for (int i = 0; i < availableDates.size(); ++i) {
            bool isSelected = (selectedDateIdx == i);
            if (ImGui::Selectable(availableDates[i].c_str(), isSelected)) {
                if (selectedDateIdx != i) {
                    selectedDateIdx = i;
                    needsRefresh = true;
                }
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    // Controls
    if (ImGui::InputInt("Merge Gap (minutes)", &mergeGapMin)) {
        if (mergeGapMin < 0) mergeGapMin = 0;
        needsRefresh = true;
    }
    
    if (ImGui::Checkbox("Hide micro sessions (< 3 seconds)", &hideMicroSessions)) {
        needsRefresh = true;
    }
    
    ImGui::Checkbox("View as Tiles (Apps View)", &tileView);
    
    ImGui::Separator();
    ImGui::Spacing();
    
    if (processedApps.empty()) {
        ImGui::Text("No data for this date.");
    } else {
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Apps View")) {
                RenderAppsView();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Timeline View")) {
                RenderTimelineView();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    
    ImGui::End();
}

void GUI::RenderAppsView() {
    ImGui::Spacing();
    ImGui::Text("Total Apps Used: %d", (int)processedApps.size());
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Approximate Total PC Usage Today: %s", LogParser::FormatDuration(totalDayUsageSec).c_str());
    ImGui::Spacing();
    
    if (!tileView) {
        // List View
        for (const auto& app : processedApps) {
            float percentage = totalDayUsageSec > 0 ? (app.totalUsageSec * 100.0f / totalDayUsageSec) : 0.0f;
            std::string header = "[ " + app.name + " ] - " + LogParser::FormatDuration(app.totalUsageSec) + " (" + std::to_string((int)percentage) + "%)";
            
            if (ImGui::CollapsingHeader(header.c_str())) {
                ImGui::Indent();
                for (size_t i = 0; i < app.sessions.size(); ++i) {
                    const auto& s = app.sessions[i];
                    std::string usage = LogParser::FormatDuration(s.endSec - s.startSec);
                    std::string startT = LogParser::FormatAMPM(s.startSec % 86400);
                    std::string endT = LogParser::FormatAMPM(s.endSec % 86400);
                    
                    ImGui::Text("Session %d: %s -> %s (Usage: %s)", (int)(i + 1), startT.c_str(), endT.c_str(), usage.c_str());
                }
                ImGui::Unindent();
                ImGui::Spacing();
            }
        }
    } else {
        // Tile View
        float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGuiStyle& style = ImGui::GetStyle();
        
        for (size_t i = 0; i < processedApps.size(); i++) {
            const auto& app = processedApps[i];
            float percentage = totalDayUsageSec > 0 ? (app.totalUsageSec * 100.0f / totalDayUsageSec) : 0.0f;
            
            ImGui::PushID((int)i);
            ImGui::BeginChild("Tile", ImVec2(280, 220), true, ImGuiWindowFlags_None);
            
            ImGui::TextWrapped("%s", app.name.c_str());
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Total Usage: %s", LogParser::FormatDuration(app.totalUsageSec).c_str());
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Daily Portion: %.1f%%", percentage);
            ImGui::TextDisabled("Total Sessions: %d", (int)app.sessions.size());
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            for (size_t sIdx = 0; sIdx < app.sessions.size(); ++sIdx) {
                const auto& s = app.sessions[sIdx];
                std::string usage = LogParser::FormatDuration(s.endSec - s.startSec);
                std::string startT = LogParser::FormatAMPM(s.startSec % 86400);
                std::string endT = LogParser::FormatAMPM(s.endSec % 86400);
                
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Session %d:", (int)(sIdx + 1));
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), " %s to %s", startT.c_str(), endT.c_str());
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.8f, 1.0f), " Duration: %s", usage.c_str());
                ImGui::Spacing();
            }
            
            ImGui::EndChild();
            ImGui::PopID();
            
            float lastButtonX2 = ImGui::GetItemRectMax().x;
            float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + 280.0f;
            if (i + 1 < processedApps.size() && nextButtonX2 < windowVisibleX2) {
                ImGui::SameLine();
            }
        }
    }
}

void GUI::RenderTimelineView() {
    ImGui::Spacing();
    ImGui::Text("Timeline Flow Analysis");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("TimelineScroll", ImVec2(0, 0), true);

    for (int i = 0; i < timelineEvents.size(); i++) {
        auto& curr = timelineEvents[i];
        
        std::string startStr = LogParser::FormatAMPM(curr.startSec % 86400);
        std::string endStr = LogParser::FormatAMPM(curr.endSec % 86400);
        std::string duration = LogParser::FormatDuration(curr.endSec - curr.startSec);

        if (i == 0) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ %s ]", curr.appName.c_str());
            ImGui::TextDisabled("%s -> %s (Duration: %s)", startStr.c_str(), endStr.c_str(), duration.c_str());
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            continue;
        }

        auto& prev = timelineEvents[i - 1];
        bool overlap = curr.startSec < prev.endSec;

        if (overlap) {
            // Overlap Highlighted
            ImGui::Text("%s", prev.appName.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), " + ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", curr.appName.c_str());
        } else {
            // Sequential Switch
            ImGui::Text("%s", prev.appName.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " -> ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", curr.appName.c_str());
        }

        ImGui::TextDisabled("%s -> %s (Duration: %s)", startStr.c_str(), endStr.c_str(), duration.c_str());
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    ImGui::EndChild();
}
