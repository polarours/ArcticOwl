# ArcticOwl

语言：中文 | [English](README.md)

> 面向实时视频监控与告警的桌面应用，基于 Qt 6、OpenCV 与 Boost.Asio 构建。


## 目录
- [ArcticOwl](#arcticowl)
  - [目录](#目录)
  - [项目概览](#项目概览)
  - [核心特性](#核心特性)
  - [架构总览](#架构总览)
  - [依赖要求](#依赖要求)
  - [构建与运行](#构建与运行)
  - [网络接口](#网络接口)
  - [项目结构](#项目结构)
  - [常见问题](#常见问题)
  - [开发者提示](#开发者提示)
  - [后续优化计划](#后续优化计划)
  - [许可证](#许可证)


## 项目概览
ArcticOwl 可用于安防、工业等场景的原型验证。它接入摄像头或 IP 视频流，执行传统视觉检测算法，将结果叠加到 Qt 界面，并通过 TCP 向客户端广播处理后的帧与警报。

当前版本：**v0.1.2**


## 核心特性
- **多种输入**：支持本地 UVC 摄像头、RTSP 流、RTMP 流。
- **传统视觉检测**：运动（MOG2/KNN）、简单入侵、火焰启发式检测。
- **实时叠加**：在 Qt 预览窗口中绘制检测框与标签。
- **TCP 广播**：向所有连接客户端推送 JPEG 帧和告警文本。
- **跨平台构建**：采用 CMake，主要在 Linux 验证，兼顾跨平台兼容性。
- **语言切换**：运行时可在英文与中文界面文本间一键切换。


## 架构总览
| 模块 | 位置 | 职责 |
| --- | --- | --- |
| Core::VideoCapture | `src/core/video_capture.*` | 抓取摄像头/RTSP/RTMP 帧，发出 `frameReady` 信号，管理线程。 |
| Core::VideoProcessor | `src/core/video_processor.*` | 执行运动、入侵、火焰检测，返回结构化结果。 |
| Modules::UI::MainWindow | `src/modules/ui/main_window.*` | Qt 界面；选择数据源、切换检测、绘制叠加、展示日志。 |
| Modules::Network::NetworkServer | `src/modules/network/network_server.*` | Boost.Asio TCP 服务（默认端口 8080），广播 JPEG 帧与警报。 |

数据流程：**视频源 → VideoCapture → VideoProcessor → UI 叠加 → NetworkServer**。

更完整的设计说明整理在 `docs/architecture/system_overview.md`（英文）。


## 依赖要求
- CMake 3.16 及以上，支持 C++17 的编译器。
- Qt 6：Core、Widgets、Network 模块。
- OpenCV：core、imgproc、highgui、imgcodecs、videoio、video、features2d。
- Boost：system、thread。
- pthread（由操作系统提供）。

Debian/Ubuntu 安装示例：
```bash
# 基础工具链
sudo apt update
sudo apt install -y build-essential cmake git

# Qt 6（包含 Core/Widgets/Network）
sudo apt install -y qt6-base-dev

# 支持 FFmpeg 的 OpenCV
sudo apt install -y libopencv-dev

# Boost 组件
sudo apt install -y libboost-system-dev libboost-thread-dev
```
提示：RTSP/RTMP 播放依赖 OpenCV 中的 FFmpeg 或 GStreamer。若系统包无法解析某些流格式，可安装额外插件或自行编译 OpenCV。


## 构建与运行
```bash
# 生成并编译（Release 示例）
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j

# 启动
./ArcticOwl
```

首次运行建议：
1. 打开程序，在“系统控制”中选择视频源。
2. 使用 RTSP/RTMP 时，在点击“启动系统”后输入流地址。
3. 按需开启运动、入侵、火焰检测。
4. 观察预览中的叠加效果；结束时点击“停止系统”。


## 网络接口
- **服务器**：`Modules::Network::NetworkServer`，默认监听 TCP 端口 8080。
- **广播内容**：
  - `broadcastFrame` → 发送带长度前缀的 JPEG 帧。
  - `sendAlert` → 发送带长度前缀的 UTF-8 告警文本。
- **数据格式**：`uint32_le payload_length` + 数据字节。当前协议未显式区分帧/告警类型，客户端需依据上下文或应用层约定识别。

最简 Python 客户端示例：
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


## 项目结构
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


## 常见问题
- **无法打开摄像头/RTSP/RTMP**
  - 检查设备权限（Linux 可将用户加入 `video` 组）和流地址连通性。
  - 确认 OpenCV 已启用 FFmpeg/GStreamer。
- **RTSP/RTMP 黑屏**
  - 安装所需编解码插件，或重新编译 OpenCV 以启用对应后端。
- **缺少 Qt "xcb" 插件**
  - 安装依赖：`sudo apt install -y libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0`。
- **8080 端口无数据**
  - 确认系统已在界面中启动，并排查防火墙限制。
- **链接 Boost 失败**
  - 重新安装 `libboost-system-dev`、`libboost-thread-dev`，并清理后再编译。


## 开发者提示
- 在 `VideoProcessor` 中扩展新检测算法后记得在 `processFrame` 汇总输出。
- 可扩展网络协议：加入消息类型字节或使用 JSON/Protobuf。
- 性能建议：丢弃过时帧、拆分 UI 与算法线程、对高分辨率流做降采样。
- `tests/` 目录预留给未来的自动化测试（例如 GoogleTest）。
- 版本管理：在 `CMakeLists.txt` 的 `project(ArcticOwl VERSION …)` 设置语义版本，构建过程会生成 `include/arctic_owl/version.h`，代码可直接读取 `ArcticOwl::Version::kString` 等常量。


## 后续优化计划
- 引入更先进的目标检测与多目标跟踪（例如 YOLOv8 搭配 SORT/ByteTrack），输出稳定的轨迹与语义标签。
- 将固定入侵区域改为用户可编辑的多边形：在界面提供绘制/编辑工具，并将配置持久化。


## 许可证
本项目遵循 MIT 许可证，详见 `LICENSE`。
