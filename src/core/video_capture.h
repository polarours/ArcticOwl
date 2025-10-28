#pragma once

#include <QObject>
#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <opencv2/opencv.hpp>

namespace ArcticOwl::Core {

class VideoCapture : public QObject
{
    Q_OBJECT

public:
    explicit VideoCapture(QObject* parent = nullptr, int camera_id = -1, const std::string& rtsp_url = "", const std::string& rtmp_url = "");
    ~VideoCapture();

    bool startVideoCaptureSystem();
    bool stopVideoCaptureSystem();

    cv::Mat getCurrentFrame();

    bool isOpened() const;

private:
    void captureLoop();

signals:
    void frameReady(const cv::Mat& frame);

private:
    int m_cameraId;
    std::string m_rtspUrl;
    std::string m_rtmpUrl;
    cv::VideoCapture m_capture;
    std::thread m_captureThread;
    std::atomic<bool> m_isRunning;
    std::atomic<int> m_pendingFrames{0};
    cv::Mat m_currentFrame;
    std::mutex m_frameMutex;
    const int m_frameIntervalMs = 33;
};

}