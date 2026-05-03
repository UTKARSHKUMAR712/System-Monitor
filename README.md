# System Usage Monitor & Viewer

A lightweight, high-performance system tracking toolkit built entirely in C++ using the native Windows API and Dear ImGui.

## 🚀 Components

The project is split into two distinct, standalone applications:

1. **Background Monitor (`monitor.exe`)**
   - Runs invisibly in the background with a ~0% CPU footprint.
   - Runs invisibly in the background with a lees than i mb of ram footprint.
   - Automatically tracks exactly which application window is actively focused on your screen.
   - Logs precise session Start/End times to daily `.txt` files inside the `logs/` folder.
   - Contains a single-instance Mutex to prevent duplicates and a custom Killswitch mechanism for safe shutdown without corrupting active logs.

2. **GUI Log Viewer (`gui_viewer.exe`)**
   - A modern, hardware-accelerated dashboard built using **Dear ImGui** and **DirectX 11**.
   - **Apps View:** Groups your daily activity into an interactive Tile layout. Displays the exact portion (percentage) of your total PC time spent in each app.
   - **Timeline View:** A flow-visualization timeline that maps out exactly how you switched between apps throughout the day, including highlighting parallel overlapping sessions (`+`).
   - Features dynamic log merging, allowing you to combine fragmented sessions that occur within a configurable timeframe (default 5 minutes).

---

## 🛠️ How to Compile

Because this project uses the native Windows API and DirectX 11, it is designed to be compiled using standard `g++` (MinGW) on Windows. No heavy frameworks (like Qt or Electron) are required.

### 1. Compiling the Monitor
```bash
g++ monitor.cpp -O2 -s -static -mwindows -o monitor.exe
```
*(Note: The `-mwindows` flag ensures the tracker detaches from the terminal and runs completely invisibly in the background).*

### 2. Compiling the GUI Viewer
The viewer requires the official Dear ImGui source files to build the interface.

**Step A: Download ImGui**  
Clone the ImGui repository directly into your project folder:
```bash
git clone https://github.com/ocornut/imgui.git
```

**Step B: Compile the Viewer**  
Run this command to compile all the custom viewer code alongside the ImGui DirectX 11 backend:
```bash
g++ main.cpp gui.cpp log_parser.cpp imgui/*.cpp imgui/backends/imgui_impl_win32.cpp imgui/backends/imgui_impl_dx11.cpp -I imgui -I . -ld3d11 -ld3dcompiler -lgdi32 -ldwmapi -O2 -static -std=c++17 -mwindows -o gui_viewer.exe
```

---

## ⚙️ How to Use

### Starting the Tracker
Simply double-click `monitor.exe`. 
To have it run automatically every day, press `Win + R`, type `shell:startup`, and place a shortcut to `monitor.exe` inside that folder. It will automatically create a `logs/` directory next to itself and begin recording your active windows silently.

### Viewing your Usage
Double-click `gui_viewer.exe`. It will automatically scan your `logs/` folder, parse the data, and present you with your total usage statistics!
