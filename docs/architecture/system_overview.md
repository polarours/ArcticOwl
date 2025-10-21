# ArcticOwl Architecture Overview

This document captures the high-level behaviour that previously lived in inline code comments. The goal is to keep the source concise while preserving the reasoning behind key design choices.

## Core Pipeline

1. **Capture** — `src/core/video_capture.cpp` launches a worker thread that reads frames from either a local device, RTSP stream, or RTMP stream. Frames are buffered minimally and dispatched to the Qt main thread through `frameReady`, keeping UI interactions responsive.
2. **Processing** — `src/core/video_processor.cpp` receives raw frames and orchestrates motion, intrusion, and fire detection. Motion detection mixes MOG2 and KNN subtractors to stabilise masks. Intrusion detection reuses motion detections inside a predefined zone, while fire detection combines colour, texture, and shape heuristics.
3. **Presentation & Alerts** — `src/modules/ui/main_window.cpp` renders annotated frames, exposes detection toggles, and simulates alert updates. When network streaming is enabled, the processed frame is also pushed to connected clients.

## Network Distribution

`src/modules/network/network_server.cpp` wraps Boost.Asio in a small TCP server. The server accepts multiple clients, pushes JPEG-encoded frames, and forwards alert strings. The implementation now validates async reads correctly and guards client lists with a mutex.

## UI Responsibilities

The Qt main window owns:
- lifecycle controls (`startSystem`, `stopSystem`),
- configuration dialogs (preferences, about),
- wiring between capture, processor, and network layers, and
- simulated alert updates via a timer.

Moving the explanations here allows the header files to stay compact while developers still understand where responsibilities sit.

## Configuration

Runtime configuration currently includes:
- capture source (local camera, RTSP, RTMP),
- network port, and
- alert refresh interval.

Expanding configuration should follow the same pattern: expose options in the preferences dialog, persist if needed, and apply on the next start.

## Future Notes

- Custom intrusion zones and networking throughput tuning remain future work for v0.1.2.
- Consider persisting preferences across sessions and exposing the network stream format.
