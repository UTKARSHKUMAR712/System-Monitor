#pragma once
#include <windows.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <string>
#include <fstream>
#include <iostream>

inline std::string GetStartupLinkPath() {
    char* appdata = getenv("APPDATA");
    if (appdata == NULL) return "";
    return std::string(appdata) + "\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\SystemMonitor.lnk";
}

inline std::string GetMonitorExePath() {
    char exePath[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exePath, MAX_PATH)) return "";
    
    std::string fullPath(exePath);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash == std::string::npos) return "";

    std::string dir = fullPath.substr(0, lastSlash);
    std::string path1 = dir + "\\monitor.exe";
    if (GetFileAttributesA(path1.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return path1;
    }
    
    size_t prevSlash = dir.find_last_of("\\/");
    if (prevSlash != std::string::npos) {
        std::string path2 = dir.substr(0, prevSlash) + "\\monitor.exe";
        if (GetFileAttributesA(path2.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return path2;
        }
    }
    
    return "";
}

inline bool IsAutoStartEnabled() {
    std::string linkPath = GetStartupLinkPath();
    if (linkPath.empty()) return false;
    if (GetFileAttributesA(linkPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    
    bool matches = false;
    HRESULT hres = CoInitialize(NULL);
    if (SUCCEEDED(hres)) {
        IShellLinkA* psl;
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID*)&psl);
        if (SUCCEEDED(hres)) {
            IPersistFile* ppf;
            hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hres)) {
                int wchars_num = MultiByteToWideChar(CP_UTF8, 0, linkPath.c_str(), -1, NULL, 0);
                wchar_t* wstr = new wchar_t[wchars_num];
                MultiByteToWideChar(CP_UTF8, 0, linkPath.c_str(), -1, wstr, wchars_num);
                
                if (SUCCEEDED(ppf->Load(wstr, STGM_READ))) {
                    char targetPath[MAX_PATH];
                    if (SUCCEEDED(psl->GetPath(targetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY))) {
                        std::string expectedPath = GetMonitorExePath();
                        if (_stricmp(targetPath, expectedPath.c_str()) == 0) {
                            matches = true;
                        }
                    }
                }
                delete[] wstr;
                ppf->Release();
            }
            psl->Release();
        }
        CoUninitialize();
    }
    return matches;
}

inline bool IsAutoStartDisabledByUser() {
    return GetFileAttributesA("disable_autostart.txt") != INVALID_FILE_ATTRIBUTES;
}

inline void SetAutoStart(bool enable) {
    std::string linkPath = GetStartupLinkPath();
    if (linkPath.empty()) return;

    if (enable) {
        remove("disable_autostart.txt");
        remove(linkPath.c_str()); // Remove old shortcut to prevent write conflicts
        
        std::string exePath = GetMonitorExePath();
        if (exePath.empty()) return;
        
        size_t lastSlash = exePath.find_last_of("\\/");
        std::string workingDir = (lastSlash != std::string::npos) ? exePath.substr(0, lastSlash) : "";

        HRESULT hres = CoInitialize(NULL);
        if (SUCCEEDED(hres)) {
            IShellLinkA* psl;
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID*)&psl);
            if (SUCCEEDED(hres)) {
                IPersistFile* ppf;
                psl->SetPath(exePath.c_str());
                psl->SetWorkingDirectory(workingDir.c_str());
                
                hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
                if (SUCCEEDED(hres)) {
                    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, linkPath.c_str(), -1, NULL, 0);
                    wchar_t* wstr = new wchar_t[wchars_num];
                    MultiByteToWideChar(CP_UTF8, 0, linkPath.c_str(), -1, wstr, wchars_num);
                    
                    ppf->Save(wstr, TRUE);
                    delete[] wstr;
                    ppf->Release();
                }
                psl->Release();
            }
            CoUninitialize();
        }
    } else {
        std::ofstream("disable_autostart.txt") << "1";
        remove(linkPath.c_str());
    }
}
