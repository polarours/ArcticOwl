# ArcticOwl API Reference

This document describes the external interfaces exposed by ArcticOwl v0.1.2 so that client applications and system integrators can consume the application safely. The desktop UI is the primary control surface; every API described here is activated only after the operator starts the system from the main window.

## 1. Network Streaming API

### 1.1 Transport
- **Protocol:** TCP
- **Default Port:** 8080 (configurable through the Preferences dialog)
- **Role:** ArcticOwl acts as TCP server, clients connect as plain sockets.
- **Socket Lifetime:** One TCP connection per client; multiple clients are supported concurrently. Connections are closed when the operator stops the system or when unrecoverable socket errors occur.

### 1.2 Message Framing
All payloads (frames and alerts) share a simple length-prefixed envelope:
```
+------------+--------------------+
| uint32_le  | payload bytes ...  |
+------------+--------------------+
```

- `uint32_le` is a 4-byte little-endian unsigned integer describing the number of bytes that follow.
- No additional checksum, compression, or type metadata is included in the envelope.

### 1.3 Frame Broadcast (`broadcastFrame`)
- **Source:** `Modules::Network::NetworkServer::broadcastFrame`
- **Trigger:** Emitted once per processed video frame while the system is running.
- **Payload:** JPEG image
  - Encoding quality is fixed at 60.
  - Resolution equals the incoming video source.
- **Consumption:** Clients should read 4 bytes for the length, then read the JPEG blob and decode it locally.

### 1.4 Alert Broadcast (`sendAlert`)
- **Source:** `Modules::Network::NetworkServer::sendAlert`
- **Trigger:** Currently invoked by the simulated alert timer in `MainWindow::updateAlerts` every ~10 iterations while the system is running. Future releases may push detections or operator-triggered alarms.
- **Payload:** UTF-8 text (no trailing null terminator)
  - Text is localized according to the operator’s language selection in the UI.

### 1.5 Example Python Client
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
        # Heuristic: treat payloads larger than 1 KB as frames.
        if size > 1024:
            open('frame.jpg', 'wb').write(data)
        else:
            print(data.decode('utf-8'))
```

### 1.6 Error Handling Expectations
- ArcticOwl does not retry on socket write errors; the connection is simply removed.
- Clients should be prepared for abrupt half-closed sockets.
- Because there is no message type byte, clients must rely on payload size or higher-level conventions to distinguish frames from alerts.

## 2. Runtime Configuration Surface

Configuration is currently transient (in-memory only) and driven from the Qt UI. The same options influence the API behaviour:

| Setting | Location | Effect on API |
| --- | --- | --- |
| Network Port | Preferences dialog → "Network Port" | TCP listener port used for frame/alert broadcast. Takes effect after the system restarts. |
| Alert Refresh Interval | Preferences dialog → "Alert Refresh Interval (ms)" | Controls how often simulated alerts are sent. |
| Video Source | System Control → Camera Settings | Determines which stream is captured and therefore what data is broadcast. |
| Language | Settings → Language | Switches all human-facing messages (including alert text) between English and Chinese. |

## 3. Detection Output Semantics

Frames delivered through the network stream contain detection overlays drawn by `Core::VideoProcessor`:
- **Intrusion:** Red bounding boxes
- **Fire:** Blue bounding boxes
- **Motion:** Green bounding boxes
- **Label Format:** `<description> (<confidence>)`

Consuming applications that need raw detection metadata should monitor future releases; v0.1.2 only exposes the rendered frame plus alert strings.

## 4. Versioning and Compatibility
- Version numbers follow Semantic Versioning and are exposed in `include/arctic_owl/version.h` as `ArcticOwl::Version::kString`.
- Breaking changes to the network framing or payload format will be documented in this file and reflected by a minor or major version bump.

## 5. Planned Extensions
- Attach message-type byte to distinguish frames vs. alerts without heuristics.
- Serialize structured detection events (JSON/Protobuf) alongside or instead of free-form alert strings.
- Persist preferences and language selection to disk.

---
For deeper architectural detail, see `docs/architecture/system_overview.md`. Bug reports and feature ideas are welcome via the project issue tracker.