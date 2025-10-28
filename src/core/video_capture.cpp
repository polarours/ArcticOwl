#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include "video_capture.h"

namespace ArcticOwl::Core {

VideoCapture::VideoCapture(QObject* parent, int camera_id, const std::string& rtsp_url, const std::string& rtmp_url)
    : QObject(parent)
    , m_cameraId(camera_id)
    , m_rtspUrl(rtsp_url)
    , m_rtmpUrl(rtmp_url)
    , m_isRunning(false)
{
}

VideoCapture::~VideoCapture()
{
    stopVideoCaptureSystem();
}

bool VideoCapture::startVideoCaptureSystem() {
    try {
        if (!m_rtspUrl.empty()) {
            m_capture.open(m_rtspUrl);
        } else if (!m_rtmpUrl.empty()) {
            m_capture.open(m_rtmpUrl);
        } else {
            m_capture.open(m_cameraId);
        }

        if (!m_capture.isOpened()){
            std::cerr << "Failed to open capture source. cameraId=" << m_cameraId
                       << ", rtspUrl=" << m_rtspUrl << ", rtmpUrl="
                       << m_rtmpUrl << std::endl;
            return false;
        }

        m_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

        m_isRunning = true;
        m_captureThread = std::thread(&VideoCapture::captureLoop, this);

        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unexpected error." << std::endl;
        return false;
    }
}

bool VideoCapture::stopVideoCaptureSystem() {
    try {
        m_isRunning = false;

        if (m_captureThread.joinable()) {
            m_captureThread.join();
        }

        if (m_capture.isOpened()) {
            m_capture.release();
        }

        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error." << std::endl;
    }

    return false;
}

cv::Mat VideoCapture::getCurrentFrame() {
    try {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (m_currentFrame.empty()) {
            return cv::Mat();
        }
        return m_currentFrame.clone();
    } catch (const std::exception& e) {
        std::cerr << "Failed to get frame: " << e.what() << std::endl;
        return cv::Mat();
    } catch (...) {
        std::cerr << "Unexpected error while getting frame" << std::endl;
        return cv::Mat();
    }
}

bool VideoCapture::isOpened() const {
    try {
        return m_capture.isOpened();
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Unexpected error." << std::endl;
        return false;
    }
}

void VideoCapture::captureLoop() {
    const auto interval = std::chrono::milliseconds(m_frameIntervalMs);

    while (m_isRunning) {
        const auto loopStart = std::chrono::steady_clock::now();

        try {
            cv::Mat frame;
            bool frame_read = m_capture.read(frame);

            if (frame_read && !frame.empty()) {
                std::lock_guard<std::mutex> lock(m_frameMutex);
                m_currentFrame = frame.clone();

                if (m_pendingFrames.load(std::memory_order_relaxed) < 2) {
                    m_pendingFrames.fetch_add(1, std::memory_order_relaxed);
                    QMetaObject::invokeMethod(this, [this, frame]() {
                            emit frameReady(frame);
                            m_pendingFrames.fetch_sub(1, std::memory_order_relaxed);
                    }, Qt::QueuedConnection);
                }
            } else if (!frame_read) {
                std::cerr << "Failed to read video frame." << std::endl;
                std::this_thread::sleep_for(interval);
            }
        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (...) {
            std::cerr << "Unexpected error." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        const auto loopEnd = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(loopEnd - loopStart);

        if (elapsed < interval) {
            std::this_thread::sleep_for(interval - elapsed);
        }
    }
}

}