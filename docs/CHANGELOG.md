# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

_No notable changes yet._

## [0.1.2] - 2025-10-21

### Added
- Language menu in the main window to toggle English/Chinese text at runtime.

### Changed
- Migrated all UI labels, dialogs, and alerts to the shared translation table.
- Documented the new language toggle in both README files.

## [0.1.1] - 2025-10-20

### Added
- Expose the `ArcticOwl` version metadata to Qt (application name/title/about dialog).
- Generate a public `include/arctic_owl/version.h` wrapper from a CMake template so code can consume version constants safely.

### Changed
- Reworked the English/Chinese README files with clearer layout and table of contents.
- Updated detection overlay labels to ASCII strings to prevent garbled characters.

### Fixed
- Resolved RTMP playback freeze caused by competing capture triggers by switching to a single-threaded capture loop with throttling/back-pressure.

## [0.1.0] - 2025-10-15

### Added
- Initial desktop release built with Qt 6, OpenCV, and Boost.Asio.
- `Core::VideoCapture` support for local camera, RTSP, and RTMP inputs.
- `Core::VideoProcessor` motion/intrusion/fire detection with real-time overlays in the Qt UI.
- `Modules::Network::NetworkServer` broadcasting JPEG frames and alert messages over TCP port 8080.
- Main window controls for start/stop, detection toggles, and alert logging.
