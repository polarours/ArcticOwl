# ArcticOwl 用户手册

ArcticOwl 是一款基于 Qt 6 的桌面程序，可实现实时视频监控、计算机视觉告警以及 TCP 帧广播。本手册提供完整的安装、操作与排错说明，帮助你在当前版本（v0.1.2）快速上手。若更习惯阅读英文，请参考 `docs/user_guide/user_guide.md`。

## 目录
- [1. 目标读者与功能概览](#1-目标读者与功能概览)
- [2. 环境要求](#2-环境要求)
- [3. 安装部署](#3-安装部署)
- [4. 首次启动步骤](#4-首次启动步骤)
- [5. 界面导览](#5-界面导览)
- [6. 操作流程](#6-操作流程)
- [7. 语言与本地化](#7-语言与本地化)
- [8. 首选项与配置](#8-首选项与配置)
- [9. 网络与客户端集成](#9-网络与客户端集成)
- [10. 使用建议](#10-使用建议)
- [11. 故障排查](#11-故障排查)
- [12. 常见问题](#12-常见问题)
- [13. 诊断与反馈](#13-诊断与反馈)
- [14. 附录](#14-附录)
- [15. 版本摘要](#15-版本摘要)

## 1. 目标读者与功能概览

ArcticOwl 面向安防原型、实验教学、视频管线开发者，核心特点包括：
- Qt Widgets 图形界面，提供摄像头控制、检测开关与告警展示。
- OpenCV 集成，实现运动、入侵与火焰检测的示例流程。
- 基于 Boost.Asio 的 TCP 广播，将 JPEG 帧和告警文本推送给多个客户端。

你可以直接用于演示，也可以在此基础上扩展检测算法、配置界面或数据持久化模块。

## 2. 环境要求

- **硬件**：至少双核 CPU、8 GB 内存。需准备 USB 摄像头或 RTSP/RTMP 网络流。GPU 仅在需要硬件加速时使用。
- **操作系统**：推荐 Linux；macOS、Windows 只要具备 Qt 6、OpenCV、Boost 环境也可运行。
- **权限**：Linux 用户务必加入 `video` 组（`sudo usermod -aG video $USER` 并重新登录），默认端口 8080 无需 root。
- **依赖**：
	- CMake ≥ 3.16，支持 C++17 的编译器。
	- Qt 6 模块：Core、Widgets、Network。
	- 带 FFmpeg 或 GStreamer 的 OpenCV，用于网络流解码。
	- Boost.System、Boost.Thread，另需 pthread（POSIX）。
- **可选工具**：Qt Linguist 用于翻译维护；若添加自动化测试可引入 GoogleTest。

## 3. 安装部署

### 3.1 克隆仓库
```bash
git clone https://github.com/polarours/ArcticOwl.git
cd ArcticOwl
```

### 3.2 Linux（Ubuntu 22.04 / Debian 系）
```bash
sudo apt update

# 构建工具与版本控制
sudo apt install -y build-essential cmake git pkg-config

# Qt 6 Widgets 相关套件
sudo apt install -y qt6-base-dev qt6-base-dev-tools qt6-tools-dev-tools

# 含多媒体后端的 OpenCV
sudo apt install -y libopencv-dev libopencv-video-dev gstreamer1.0-libav

# Boost 线程组件
sudo apt install -y libboost-system-dev libboost-thread-dev

# 可选：Qt Linguist 翻译工具
sudo apt install -y qt6-tools-dev qt6-translations
```
若 Qt 提示缺少 `xcb` 插件，可补充安装：`sudo apt install -y libxkbcommon-x11-0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0`。

### 3.3 Linux（Manjaro / Arch 系）
```bash
sudo pacman -Syu --needed base-devel git cmake pkgconf

# Qt 6 库与工具
sudo pacman -S --needed qt6-base qt6-tools qt6-translations

# OpenCV 与编解码依赖
sudo pacman -S --needed opencv ffmpeg gstreamer libgstreamer

# Boost 库（打包为单个软件包）
sudo pacman -S --needed boost
```
如遇 pacman 与旧版 Qt 冲突，可先卸载旧包或在干净的 chroot 中安装。

### 3.4 macOS 13+（Apple Silicon 或 Intel）
```bash
brew update
brew install cmake git qt opencv boost

# 让 CMake 自动找到 Homebrew 安装的 Qt
echo 'export CMAKE_PREFIX_PATH="$(brew --prefix qt)"' >> ~/.zshrc
source ~/.zshrc
```
若在构建目录外启动应用，需要先设置：
```bash
export QT_PLUGIN_PATH="$(brew --prefix qt)/lib/qt/plugins"
```

### 3.5 Windows 11（MSVC + vcpkg）
1. 安装 **Visual Studio 2022** 并勾选 *Desktop development with C++* 工作负载。
2. 安装 **CMake** 与 **Git for Windows**。
3. 初始化 vcpkg：
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\tools\vcpkg
   C:\tools\vcpkg\bootstrap-vcpkg.bat
   ```
4. 安装依赖：
   ```powershell
   C:\tools\vcpkg\vcpkg.exe install qtbase opencv4 boost-system boost-thread --triplet x64-windows
   ```
5. 使用 vcpkg 工具链生成与编译：
   ```powershell
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --parallel
   ```
如需 FFmpeg 支持，可在 vcpkg 安装命令中追加 `ffmpeg`，或在配置阶段启用 GStreamer。

### 3.6 通用编译与运行
```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j

# 构建完成后
./ArcticOwl
```

### 3.7 升级到新版本

- 在仓库根目录执行 `git pull`，随后在 `build/` 中运行 `cmake --build . -j`。
- 如遇 CMake、依赖变化，请重新执行 `cmake ..` 后再编译。
- 切换分支或大版本升级时，建议按照附录 14.1 进行一次干净构建。

## 4. 首次启动步骤

1. 在 `build/` 或你的安装路径执行 `./ArcticOwl`。
2. 确认主界面展示：系统控制区、视频预览、检测开关、告警日志、摄像头列表。
3. 若需中文或英文界面，可立即在菜单 **Language** 中切换。
4. 启动前请确认摄像头连接稳定或网络地址可访问。

> **提示**：首次运行建议保留终端窗口，可快速查看启动日志与错误信息。

## 5. 界面导览

| 区域 | 功能说明 | 常见操作 |
| --- | --- | --- |
| 菜单栏 | 提供设置、语言、帮助入口 | 打开首选项、切换语言、查看 About |
| 系统控制 | 启动/停止按钮与视频源选择 | 设定摄像头编号，选择本地/RTSP/RTMP，启动管线 |
| 检测设置 | 运动、入侵、火焰检测开关 | 根据场景启用或关闭检测模块 |
| 视频预览 | 显示实时画面与检测标注 | 观察识别结果或状态文本 |
| 告警日志 | 健康检查与模拟告警输出 | 查看系统心跳，验证定时器执行 |
| 摄像头表格 | 预留摄像头管理入口 | 后续可扩展为摄像头清单与状态面板 |

键盘可使用 Tab/Shift+Tab 在控件间切换焦点，满足无鼠标操作需求。

## 6. 操作流程

### 6.1 启动系统

1. 选择摄像头来源：本地设备可设定 ID，网络流选择 RTSP 或 RTMP。
2. 点击 **Start System**，若需网络地址则按提示输入。
3. 状态文本显示 “Acquiring video stream…” 直到收到第一帧。
4. 检测模块的初始状态取决于复选框勾选情况，可随时切换。

### 6.2 监控检测结果

- **运动检测**：使用背景差分（MOG2/KNN）突出运动目标。
- **入侵检测**：关注预设区域内的运动结果（未来计划支持自定义区域）。
- **火焰检测**：基于颜色与轮廓特征判断火焰，反光物体可能误报。
- 勾选状态即时生效，无需停止系统。

### 6.3 告警面板

- 计时器会周期性写入模拟告警，辅助确认系统仍在运行。
- 若需清空，可停止后重新启动。未来版本将加入手动清除按钮。

### 6.4 停止系统

- 点击 **Stop System** 以释放摄像头、停止网络广播与告警计时器。
- 在拔出设备或退出程序前务必先停止系统，可避免操作系统残留占用。
- 若停止失败，界面会提示警告，系统仍保持可控状态以便再次尝试。

## 7. 语言与本地化

- ArcticOwl 默认包含英文与简体中文翻译文件。
- 在 **Settings → Language** 中即可切换，实时生效，无需重启。
- 当前语言设置仅对本次会话有效，未来将加入跨会话记忆功能。
- 翻译源文件位于 `resources/translations/`，编译时转为 `.qm` 文件打包到资源中。

## 8. 首选项与配置

- 通过 **Settings → Preferences…** 打开首选项对话框。
- 当前参数：
	- **Network Port**：1024–65535。为避免中断客户端连接，端口变更会在下次启动系统时生效。
	- **Alert Refresh Interval (ms)**：200–10000。修改后即时更新告警计时器。
- 设置暂存于内存中，若需持久化可扩展 `QSettings` 或自定义配置文件。
- **About ArcticOwl** 对话框展示版本号（来源 `include/arctic_owl/version.h`）、技术栈与项目地址。

## 9. 网络与客户端集成

- 默认监听 TCP 8080 端口，部署时记得在防火墙放行。
- 发送格式：先写入小端 32 位长度，再写入内容。
- 内容类型包括：
	- JPEG 帧字节流；
	- UTF-8 告警文本。
- Python 客户端示例：
	```python
	import socket, struct

	with socket.create_connection(("127.0.0.1", 8080)) as sock:
			size = struct.unpack('<I', sock.recv(4))[0]
			payload = sock.recv(size)
			open('frame.jpg', 'wb').write(payload)
	```
- 若需区分消息类型，可在上层协议中引入额外标记或通道字段。

## 10. 使用建议

- **先在 VLC/ffplay 验证视频源**，确保地址和编码正确。
- **从低分辨率开始调试**，确认稳定后再提升分辨率与帧率。
- **将 ArcticOwl 部署在独立主机**，避免与高负载任务竞争资源。
- **开发新检测功能时认真监控日志**，以便快速定位 OpenCV 异常。
- **如扩展配置界面**，建议引入版本化配置文件方便回滚。

## 11. 故障排查

| 现象 | 可能原因 | 解决方案 |
| --- | --- | --- |
| 预览始终黑屏 | 解码器缺失或流不可达 | 安装 FFmpeg/GStreamer，先用 VLC 验证地址，再重启 ArcticOwl。 |
| “Failed to start the camera.” | 摄像头被占用或权限不足 | 确保无其他程序占用摄像头；将用户加入 `video` 组后重新登录。 |
| 客户端收不到数据 | 服务器未启动或端口被阻挡 | 确认系统已启动，检查首选项端口设置，放行防火墙规则。 |
| CPU 占用过高 | 高分辨率 + 全部检测模块 | 降低输入分辨率或帧率，关闭不需要的检测模块，必要时升级硬件。 |
| 语言切换无效 | `.qm` 文件缺失 | 检查 `build/translations/arcticowl_zh_CN.qm` 是否存在，必要时重新构建。 |
| 构建时 `lrelease` 报错 | `.ts` 文件包含旧标签 | 删除过时标签（例如 `<defaultcodec>`），重新运行 CMake 生成 `.qm`。 |

> **提醒**：拔除摄像头前务必先停止系统，避免操作系统保持占用状态。

## 12. 常见问题

- **是否支持录像？** 当前版本不提供记录功能，后续会考虑增加录制与回放能力。
- **入侵区域能否自定义？** v0.1.2 为固定区域，未来会提供可编辑多边形。
- **是否有纯命令行模式？** 暂无。Qt GUI 负责驱动采集与计时器，必须保持前台运行。
- **支持哪些编码格式？** 只要 OpenCV 能处理的流都可使用（UVC 摄像头、RTSP H.264/H.265、RTMP、MJPEG 等）。
- **如何扩展检测算法？** 在 `Core::VideoProcessor` 编写新逻辑，返回结构化结果并在 `MainWindow::updateFrame` 中渲染。

## 13. 诊断与反馈

1. 在程序运行中复现问题。
2. 保存终端输出和 Qt 弹窗信息，必要时录屏或截图。
3. 记录环境：操作系统版本、Qt 版本（`qmake -query QT_VERSION`）、OpenCV 版本（`pkg-config --modversion opencv4`）、显卡驱动版本（如涉及硬件加速）。
4. 如问题与特定视频流相关，请提供样例帧或短视频。
5. 在仓库提交 Issue 或联系维护者，并附上以上资料与复现步骤。

## 14. 附录

### 14.1 清理构建目录

```bash
cd build
cmake --build . --target clean
rm -rf *
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

### 14.2 目录速查

- `src/core/`：视频采集与处理模块。
- `src/modules/ui/`：Qt 界面逻辑及语言切换实现。
- `resources/translations/`：翻译源 `.ts` 与编译后的 `.qm`。
- `docs/`：架构说明、API 文档、用户手册等资料。

## 15. 版本摘要

- **当前版本**：v0.1.2
- **重点更新**：运行时语言切换、焕新 UI 布局、文档扩充、Qt Linguist 翻译流程。
- **后续计划**：配置持久化、自定义入侵区域、更完善的告警管理。

希望本中文版手册能帮助你和团队快速部署并熟练使用 ArcticOwl，必要时可随时切换到英文文档获取更多细节。
