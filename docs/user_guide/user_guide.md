# ArcticOwl User Guide

ArcticOwl is a Qt 6 desktop application for real-time video monitoring, computer-vision based alerting, and TCP frame streaming. Use this guide to install the software, understand the interface, operate detectors, and troubleshoot deployments. A Chinese translation is available in `docs/user_guide/user_guide.zh-CN.md` so teams can switch to whichever language they prefer.

## Contents
- [ArcticOwl User Guide](#arcticowl-user-guide)
  - [Contents](#contents)
  - [1. Overview and Audience](#1-overview-and-audience)
  - [2. Requirements](#2-requirements)
  - [3. Installation](#3-installation)
    - [3.1 Clone the Repository](#31-clone-the-repository)
    - [3.2 Linux (Ubuntu 22.04 / Debian Family)](#32-linux-ubuntu-2204--debian-family)
    - [3.3 Linux (Manjaro / Arch Family)](#33-linux-manjaro--arch-family)
    - [3.4 macOS 13+ (Apple Silicon or Intel)](#34-macos-13-apple-silicon-or-intel)
    - [3.5 Windows 11 (MSVC + vcpkg)](#35-windows-11-msvc--vcpkg)
    - [3.6 Build and Launch (All Platforms)](#36-build-and-launch-all-platforms)
    - [3.7 Updating to New Releases](#37-updating-to-new-releases)
  - [4. First Launch](#4-first-launch)
  - [5. Interface Tour](#5-interface-tour)
  - [6. Operating Workflow](#6-operating-workflow)
    - [6.1 Starting the System](#61-starting-the-system)
    - [6.2 Monitoring Detections](#62-monitoring-detections)
    - [6.3 Handling Alerts](#63-handling-alerts)
    - [6.4 Stopping the System](#64-stopping-the-system)
  - [7. Language and Localization](#7-language-and-localization)
  - [8. Preferences and Configuration](#8-preferences-and-configuration)
  - [9. Networking and Clients](#9-networking-and-clients)
  - [10. Best Practices](#10-best-practices)
  - [11. Troubleshooting](#11-troubleshooting)
  - [12. FAQ](#12-faq)
  - [13. Diagnostics and Support](#13-diagnostics-and-support)
  - [14. Appendices](#14-appendices)
    - [14.1 Clean Rebuild Procedure](#141-clean-rebuild-procedure)
    - [14.2 Directory Cheat Sheet](#142-directory-cheat-sheet)
  - [15. Release Notes Snapshot](#15-release-notes-snapshot)

## 1. Overview and Audience

ArcticOwl targets security prototyping teams, lab demonstrations, and developers who need a reference implementation of a multi-source video pipeline. It ships with:
- A Qt Widgets interface for camera control and alert monitoring.
- OpenCV integration for motion, intrusion, and basic flame detection.
- A Boost.Asio TCP broadcaster that streams JPEG frames and alert strings.

The application balances approachability and extensibility. You can use it out-of-the-box for demos or extend it with new detectors, persistence layers, or UI panels.

## 2. Requirements

- **Hardware:** Dual-core CPU minimum, 8 GB RAM recommended. USB camera or RTSP/RTMP source. GPU only required for advanced OpenCV builds.
- **Operating systems:** Linux (primary), macOS, Windows. Ensure Qt 6, OpenCV, and Boost packages are available on your target OS.
- **Permissions:** Linux users should join the `video` group to access cameras (`sudo usermod -aG video $USER`, then relog). Ports below 1024 require elevated privileges; default port 8080 is unprivileged.
- **Dependencies:**
  - CMake 3.16 or newer with a C++17 toolchain.
  - Qt 6 modules: Core, Widgets, Network.
  - OpenCV with FFmpeg or GStreamer support for network streams.
  - Boost.System and Boost.Thread, plus pthread on POSIX systems.
- **Optional tooling:** Qt Linguist for editing translations, GoogleTest if you expand tests.

## 3. Installation

### 3.1 Clone the Repository
```bash
git clone https://github.com/polarours/ArcticOwl.git
cd ArcticOwl
```

### 3.2 Linux (Ubuntu 22.04 / Debian Family)
```bash
sudo apt update

# Build tools and version control
sudo apt install -y build-essential cmake git pkg-config

# Qt 6 widgets stack
sudo apt install -y qt6-base-dev qt6-base-dev-tools qt6-tools-dev-tools

# OpenCV with multimedia backends
sudo apt install -y libopencv-dev libopencv-video-dev gstreamer1.0-libav

# Boost threading components
sudo apt install -y libboost-system-dev libboost-thread-dev

# Optional: Qt Linguist for translation editing
sudo apt install -y qt6-tools-dev qt6-translations
```
If Qt reports missing `xcb` plugins, add: `sudo apt install -y libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0`.

### 3.3 Linux (Manjaro / Arch Family)
```bash
sudo pacman -Syu --needed base-devel git cmake pkgconf

# Qt 6 libraries and tools
sudo pacman -S --needed qt6-base qt6-tools qt6-translations

# OpenCV with codec support
sudo pacman -S --needed opencv ffmpeg gstreamer libgstreamer

# Boost libraries (single package)
sudo pacman -S --needed boost
```
If pacman conflicts with legacy Qt packages, remove them or install ArcticOwl inside a clean chroot.

### 3.4 macOS 13+ (Apple Silicon or Intel)
```bash
brew update
brew install cmake git qt opencv boost

# Let CMake locate Homebrew Qt automatically
echo 'export CMAKE_PREFIX_PATH="$(brew --prefix qt)"' >> ~/.zshrc
source ~/.zshrc
```
When launching the app outside the build tree, export:
```bash
export QT_PLUGIN_PATH="$(brew --prefix qt)/lib/qt/plugins"
```

### 3.5 Windows 11 (MSVC + vcpkg)
1. Install **Visual Studio 2022** with the *Desktop development with C++* workload.
2. Install **CMake** and **Git for Windows**.
3. Bootstrap vcpkg:
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\tools\vcpkg
   C:\tools\vcpkg\bootstrap-vcpkg.bat
   ```
4. Install dependencies:
   ```powershell
   C:\tools\vcpkg\vcpkg.exe install qtbase opencv4 boost-system boost-thread --triplet x64-windows
   ```
5. Configure and build via the vcpkg toolchain:
   ```powershell
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --parallel
   ```
If you need FFmpeg-enabled OpenCV, append `ffmpeg` to the vcpkg install command or enable GStreamer support manually.

### 3.6 Build and Launch (All Platforms)
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j

# After a successful build
./ArcticOwl
```

### 3.7 Updating to New Releases

- Fetch changes (`git pull`), then rebuild from the existing `build/` directory using `cmake --build . -j`.
- If CMake scripts or dependencies changed, rerun `cmake ..` before rebuilding.
- When switching branches or performing major upgrades, run a clean build (see Appendix 14.1).

## 4. First Launch

1. Start the executable from the `build/` directory or from your installation path.
2. Verify that the main window displays these regions: System Control bar, video preview, detection toggles, Alert log, camera table.
3. Choose the UI language in **Language → English/Chinese** if desired.
4. Confirm your video source (USB camera or stream URL) is connected and reachable before starting.

> **Tip:** Keep the console visible on first launch. ArcticOwl prints detailed error messages when a stream or detector fails to initialize.

## 5. Interface Tour

| Area | Description | Key actions |
| --- | --- | --- |
| Menu Bar | Access to Settings, Language, Help | Open Preferences, switch languages, view About dialog |
| System Control Group | Start/Stop buttons and camera source selection | Start pipelines, choose camera ID, select Local/RTSP/RTMP source |
| Detection Group | Toggles for Motion, Intrusion, Fire | Enable or disable detectors per session |
| Video Preview | Scaled live feed with overlays | Monitor detections; shows status text when idle |
| Alerts Log | Scrollable history of simulated alerts | Observe activity pulses; useful for health checks |
| Camera Table | Placeholder list of configured cameras | Extend in future versions for fleet management |

All controls are accessible with keyboard navigation. Focus moves between widgets using Tab/Shift+Tab.

## 6. Operating Workflow

### 6.1 Starting the System

1. Set the camera ID (for local devices) or select the RTSP/RTMP option in the source combo.
2. Click **Start System**. Provide stream URLs when prompted.
3. The status label reads *“Acquiring video stream...”* until frames arrive. Once connected, overlays appear.
4. Detectors inherit the state of their checkboxes at start time.

### 6.2 Monitoring Detections

- **Motion Detection** uses background subtraction (MOG2/KNN) to highlight moving objects.
- **Intrusion Detection** checks for motion within a predefined zone (custom zones are on the roadmap).
- **Fire Detection** inspects color and contour heuristics; reflective surfaces can trigger false positives.
- Toggle detectors on or off at any time; changes propagate immediately.

### 6.3 Handling Alerts

- The alert pane appends status entries every few timer ticks to mimic downstream integrations.
- Use the log to confirm the timer is active and that the system remains responsive.
- To clear the log, stop and restart the system. A manual clear button is planned for a later release.

### 6.4 Stopping the System

- Press **Stop System** before disconnecting cameras or closing the app.
- Stopping flushes timers, stops network broadcasting, and releases capture handles.
- If stop fails, the UI shows a warning but continues to run so you can retry or exit cleanly.

## 7. Language and Localization

- ArcticOwl bundles English and Simplified Chinese translations.
- Language selection lives under **Settings → Language**.
- Switching languages triggers Qt’s translation machinery instantly; no restart is required.
- The chosen language applies for the active session. Persistence across restarts is slated for a future milestone.
- Translation updates are stored under `resources/translations/` and compiled into `.qm` files during the build.

## 8. Preferences and Configuration

- Open **Settings → Preferences…** to adjust runtime parameters.
- Fields currently available:
  - **Network Port:** Range 1024–65535. Changes queue until the next system start to avoid disconnecting active clients midstream.
  - **Alert Refresh Interval (ms):** Range 200–10000. Applied immediately to the alert timer.
- All settings are stored in-memory. If you require persistence, extend the dialog to save values via `QSettings` or a custom config file.
- The **About ArcticOwl** dialog lists the version string from `include/arctic_owl/version.h`, technology stack, and the project URL.

## 9. Networking and Clients

- The TCP broadcaster listens on port 8080 by default. Configure firewalls to allow inbound connections on this port.
- Each payload is prefixed with a 32-bit little-endian length. Clients should read the length, then read the payload.
- Message types:
  - JPEG frame bytes (default stream).
  - UTF-8 encoded alert strings.
- Minimal Python client (excerpt):
  ```python
  import socket, struct

  with socket.create_connection(("127.0.0.1", 8080)) as sock:
		size = struct.unpack('<I', sock.recv(4))[0]
		payload = sock.recv(size)
		open('frame.jpg', 'wb').write(payload)
  ```
- For production scenarios, consider adding a message header or channel tag so clients can distinguish frame data from alert text.

## 10. Best Practices

- **Validate streams with VLC or ffplay** before connecting them to ArcticOwl.
- **Start with lower resolutions** to confirm throughput, then scale up gradually.
- **Separate workloads** by running ArcticOwl on a dedicated machine if detectors consume significant CPU.
- **Monitor logs** when developing new detectors to catch OpenCV exceptions early.
- **Version your configuration** (e.g., YAML files) when extending the preferences dialog.

## 11. Troubleshooting

| Symptom | Likely Cause | Resolution |
| --- | --- | --- |
| Preview stays black | Codec missing or stream unreachable | Install FFmpeg/GStreamer support. Test the URL in VLC. Restart ArcticOwl after verifying credentials. |
| “Failed to start the camera.” | Device busy or permission issue | Ensure no other app uses the camera. On Linux, add the user to the `video` group and relogin. |
| No data on TCP clients | Server not running or port blocked | Confirm the system is started. Check the network port in Preferences. Allow the port through local firewalls. |
| CPU usage spikes | High-resolution feed with all detectors enabled | Reduce resolution/frame rate. Disable detectors not required for the session. Upgrade hardware if needed. |
| Language menu has no effect | `.qm` translation file missing | Verify `build/translations/arcticowl_zh_CN.qm` exists or rebuild the project. |
| Build fails at `lrelease` | Outdated `.ts` schema | Remove legacy tags (e.g., `<defaultcodec>`) and rerun CMake to regenerate `.qm`. |

> **Reminder:** Always stop the system before unplugging cameras to prevent the OS from leaving devices in a busy state.

## 12. FAQ

- **Does ArcticOwl record video?** Not in v0.1.2. Recording and replay tooling are on the roadmap.
- **Can I define custom intrusion zones?** Zones are fixed today. Editable polygons will arrive in a future release.
- **Is headless mode available?** No. The Qt GUI orchestrates capture and timers, so the app must stay visible.
- **Which codecs are supported?** Any stream OpenCV handles: USB UVC, RTSP (H.264/H.265), RTMP, MJPEG, etc., subject to your OpenCV build.
- **How do I extend detectors?** Implement new logic in `Core::VideoProcessor`, emit structured results, and render them in `MainWindow::updateFrame`.

## 13. Diagnostics and Support

1. Reproduce the issue with ArcticOwl running.
2. Collect console output and any Qt dialog messages.
3. Note system details: OS release, Qt version (`qmake -query QT_VERSION`), OpenCV version (`pkg-config --modversion opencv4`), GPU driver (if relevant).
4. Capture sample frames or videos if the bug is content-specific.
5. Open a GitHub issue or contact maintainers with the gathered data. Include reproduction steps and whether the issue occurs in both languages.

## 14. Appendices

### 14.1 Clean Rebuild Procedure

```bash
cd build
cmake --build . --target clean
rm -rf *
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

### 14.2 Directory Cheat Sheet

- `src/core/`: Video capture and processing modules.
- `src/modules/ui/`: Qt UI implementation, including language switching.
- `resources/translations/`: `.ts` sources and compiled `.qm` files.
- `docs/`: Additional documentation (architecture overview, API notes, localized guides).

## 15. Release Notes Snapshot

- **Current version:** v0.1.2
- **Highlights:** Runtime language switching, refreshed UI layout, updated documentation, Qt Linguist translation pipeline.
- **Next steps:** Config persistence, custom intrusion zones, richer alert management.

Use this guide alongside the localized Chinese version to onboard teammates quickly, regardless of preferred language.
