# ArcticOwl API 参考（简体中文）

本文档覆盖 ArcticOwl v0.1.2 对外开放的接口，方便客户端应用和系统集成方安全地消费数据。桌面 UI 是主要控制入口，只有当操作员在主窗口中启动系统后，以下 API 才会对外工作。

## 1. 网络流媒体接口

### 1.1 传输层
- **协议：** TCP
- **默认端口：** 8080（可在“首选项”对话框中修改）
- **角色分工：** ArcticOwl 充当 TCP 服务器，客户端以普通套接字连接。
- **连接生命周期：** 每个客户端一个 TCP 连接，可同时支持多个连接。当操作者停止系统或发生不可恢复的套接字错误时，连接会被关闭。

### 1.2 消息封装
所有负载（视频帧与告警）均使用简单的长度前缀封装：
```
+------------+--------------------+
| uint32_le  | payload bytes ...  |
+------------+--------------------+
```

- `uint32_le` 为 4 字节小端无符号整数，指明后续负载的字节数。
- 封装中不包含额外的校验和、压缩或类型元数据。

### 1.3 帧广播（`broadcastFrame`）
- **来源：** `Modules::Network::NetworkServer::broadcastFrame`
- **触发：** 系统运行时，每处理完一帧视频便广播一次。
- **负载：** JPEG 图像
  - 编码质量固定为 60。
  - 分辨率与输入视频源保持一致。
- **消费方式：** 客户端先读取 4 字节长度，再读取 JPEG 数据并在本地解码。

### 1.4 告警广播（`sendAlert`）
- **来源：** `Modules::Network::NetworkServer::sendAlert`
- **触发：** 目前由 `MainWindow::updateAlerts` 的模拟告警定时器驱动，大约每 10 次迭代推送一次。后续版本可能发送真实检测事件或人工告警。
- **负载：** UTF-8 文本（无结尾空字符）
  - 文本内容根据 UI 中的语言选择自动本地化。

### 1.5 Python 客户端示例
```python
import socket
import struct

HOST, PORT = "127.0.0.1", 8080
with socket.create_connection((HOST, PORT)) as conn:
	while True:
		header = conn.recv(4)
		if len(header) < 4:
			break
		(size,) = struct.unpack('<I', header)
		data = bytearray()
		while len(data) < size:
			chunk = conn.recv(size - len(data))
			if not chunk:
				break
			data.extend(chunk)
		# 简单启发：大于 1 KB 视为帧，否则视为告警文本。
		if size > 1024:
			open('frame.jpg', 'wb').write(data)
		else:
			print(data.decode('utf-8'))
```

### 1.6 错误处理期望
- ArcticOwl 不会对写入失败的套接字做重试，连接会被移除。
- 客户端需处理半关闭或突然断开的连接。
- 由于封装中没有显式类型字节，客户端需通过负载大小或上层约定区分帧与告警。

## 2. 运行时配置接口

目前配置均为内存态，由 Qt UI 驱动，相同设置也会影响 API 行为：

| 设置项 | UI 位置 | 对 API 的影响 |
| --- | --- | --- |
| 网络端口 | 首选项 → “Network Port” | 控制 TCP 监听端口，需重新启动系统才生效。 |
| 告警刷新间隔 | 首选项 → “Alert Refresh Interval (ms)” | 决定模拟告警推送频率。 |
| 视频源 | 系统控制 → Camera Settings | 决定采集与广播的数据来源。 |
| 语言 | 设置 → Language | 切换所有面向用户的文本（包括告警字符串）的语言。 |

## 3. 检测结果语义

通过网络流发送的帧已经叠加了 `Core::VideoProcessor` 绘制的检测信息：
- **Intrusion（入侵）：** 红色框
- **Fire（火焰）：** 蓝色框
- **Motion（运动）：** 绿色框
- **标签格式：** `<description> (<confidence>)`

如需获取原始检测元数据，请关注后续版本；v0.1.2 仅提供渲染后的帧与告警文本。

## 4. 版本管理与兼容性
- 版本号遵循 SemVer，并在 `include/arctic_owl/version.h` 中以 `ArcticOwl::Version::kString` 暴露。
- 若网络封装或负载格式发生不兼容变更，会在本文档中记录，并通过次版本或主版本号提升告知。

## 5. 未来计划
- 添加消息类型字节，避免客户端依赖负载大小进行区分。
- 提供结构化检测事件（JSON/Protobuf），替代或补充自由文本告警。
- 将首选项和语言选择持久化到磁盘。

---
更多架构背景请参阅 `docs/architecture/system_overview.md`。若有问题或新需求，欢迎在项目 Issue 中交流。
