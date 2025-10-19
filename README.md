# ArcticOwl

Language: English | [中文](README_zh_CN.md)

> Real-time video monitoring and alerting desktop app powered by Qt 6, OpenCV, and Boost.Asio.


## Contents
- [Overview](#overview)
- [Highlights](#highlights)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Build and Run](#build-and-run)
- [Network Interface](#network-interface)
- [Project Structure](#project-structure)
- [FAQ](#faq)
- [Development Notes](#development-notes)
- [Planned Improvements](#planned-improvements)
- [License](#license)


## Overview
ArcticOwl is a desktop application designed for security and industrial monitoring prototypes. It ingests live video from cameras or IP streams, applies classic computer-vision detectors, overlays results in a Qt UI, and broadcasts processed frames plus alert messages to TCP clients.


## Highlights
- **Multiple sources**: local UVC camera, RTSP stream, or RTMP stream.
- **Classic CV detection**: motion (MOG2/KNN), simple intrusion check, flame heuristics.
- **Live overlay**: bounding boxes and labels rendered directly in the Qt window.
- **TCP broadcasting**: JPEG frames and alert strings pushed to all connected clients.
- **Portable build**: CMake-based, tested mainly on Linux but kept cross-platform friendly.


## Architecture
| Module | Location | Responsibilities |
| --- | --- | --- |
| Core::VideoCapture | `src/core/video_capture.*` | Capture frames from camera/RTSP/RTMP, emit `frameReady` signal, manage capture thread. |
| Core::VideoProcessor | `src/core/video_processor.*` | Run motion, intrusion, and fire detection, return structured results. |
| Modules::UI::MainWindow | `src/modules/ui/main_window.*` | Qt interface: source selection, detection toggles, drawing overlays, log panel. |
| Modules::Network::NetworkServer | `src/modules/network/network_server.*` | Boost.Asio TCP server (default 8080) broadcasting JPEG frames and alerts. |

Data path: **Video Source → VideoCapture → VideoProcessor → UI Overlay → NetworkServer**.


## Requirements
- CMake 3.16 or newer, C++17 toolchain.
- Qt 6: Core, Widgets, Network modules.
- OpenCV: core, imgproc, highgui, imgcodecs, videoio, video, features2d.
- Boost: system, thread.
- pthread (provided by the OS).

Debian/Ubuntu quick setup:
```bash
# Toolchain
sudo apt update
sudo apt install -y build-essential cmake git

# Qt 6 (Core/Widgets/Network)
sudo apt install -y qt6-base-dev

# OpenCV with FFmpeg support
sudo apt install -y libopencv-dev

# Boost libraries
sudo apt install -y libboost-system-dev libboost-thread-dev
```
Note: RTSP/RTMP playback needs an OpenCV build with FFmpeg or GStreamer enabled. If the stock packages cannot handle your stream, install the appropriate multimedia plugins or rebuild OpenCV.


## Build and Run
```bash
# Configure and compile (Release example)
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j

# Launch
./ArcticOwl
```

First run checklist:
1. Start the application and pick a video source in the System Controls group.
2. For RTSP/RTMP, provide the URL when prompted after pressing Start System.
3. Toggle motion, intrusion, and fire detection as required.
4. Observe overlays in the preview; Stop System when finished.


## Network Interface
- **Server**: `Modules::Network::NetworkServer`, listening on TCP port 8080 by default.
- **Broadcasts**:
  - `broadcastFrame` → JPEG frame with length prefix.
  - `sendAlert` → UTF-8 alert text with length prefix.
- **Wire format**: `uint32_le payload_length` + payload bytes. The current protocol does not encode message type; clients must infer it by context or use per-channel conventions.

Minimal Python client example:
```python
import socket, struct

with socket.create_connection(("127.0.0.1", 8080)) as s:
    size_bytes = s.recv(4)
    (size,) = struct.unpack('<I', size_bytes)
    data = bytearray()
    while len(data) < size:
        chunk = s.recv(size - len(data))
        if not chunk:
            break
        data.extend(chunk)
    open('frame.jpg', 'wb').write(data)
```


## Project Structure
```
CMakeLists.txt
src/
  main.cpp
  core/
    video_capture.{h,cpp}
    video_processor.{h,cpp}
  modules/
    ui/main_window.{h,cpp}
    network/network_server.{h,cpp}
include/
  arctic_owl/version.h
```


## FAQ
- **Cannot open camera/RTSP/RTMP**
  - Verify device permissions (Linux: add user to `video` group) or stream URL reachability.
  - Ensure OpenCV has FFmpeg/GStreamer support.
- **Black video for RTSP/RTMP**
  - Install additional codecs/plugins or rebuild OpenCV with the needed backends.
- **Qt platform plugin "xcb" missing**
  - Install runtime deps: `sudo apt install -y libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0`.
- **Port 8080 shows no data**
  - Start the system from the UI and confirm no firewall blocks the port.
- **Boost link errors**
  - Reinstall `libboost-system-dev` and `libboost-thread-dev`, then clean build.


## Development Notes
- Add new detectors inside `VideoProcessor` and aggregate results in `processFrame`.
- Consider extending the network wire format with a message type byte or a higher-level protocol (JSON/Protobuf).
- Performance tips: drop unused frames, decouple UI and processing threads, and explore downsampling for heavy streams.
- The `tests/` directory is reserved for future automated tests (GoogleTest or similar).


## Planned Improvements
- Integrate a modern object detector plus multi-object tracker (e.g., YOLOv8 with SORT or ByteTrack) to output stable trajectories and semantic labels.
- Replace the fixed intrusion rectangle with user-defined polygons editable in the UI and persisted to configuration files.


## License
This project is licensed under the MIT License. See `LICENSE`.
