#ifndef ARCTICOWL_CORE_VIDEO_CAPTURE_H
#define ARCTICOWL_CORE_VIDEO_CAPTURE_H

#include <QObject>
#include <atomic>
#include <string>
#include <thread>
#include <opencv2/opencv.hpp>

namespace ArcticOwl {
namespace Core {

/**
 * @brief The VideoCapture class
 * @details 负责从摄像头或RTSP/RTMP流中捕获视频帧，并通过信号将帧传递给其他组件
 */
class VideoCapture : public QObject
{
    Q_OBJECT

public:
    // 构造函数
    explicit VideoCapture(QObject* parent = nullptr, int camera_id = -1, const std::string& rtsp_url = "", const std::string& rtmp_url = "");
    // 析构函数
    ~VideoCapture();

    // 启动视频捕获系统
    bool startVideoCaptureSystem();
    // 停止视频捕获系统
    bool stopVideoCaptureSystem();

    // 获取当前捕获的帧
    cv::Mat getCurrentFrame();

    // 检查视频捕获是否已打开
    bool isOpened() const;

private:
    // 捕获视频帧的线程函数
    void captureLoop();

signals:
    // 当新帧准备好时发出信号
    void frameReady(const cv::Mat& frame);

private:
    int m_cameraId;                           ///< 摄像头ID
    std::string m_rtspUrl;                    ///< RTSP流URL
    std::string m_rtmpUrl;                    ///< RTMP流URL
    cv::VideoCapture m_capture;               ///< OpenCV视频捕获对象
    std::thread m_captureThread;              ///< 捕获线程
    std::atomic<bool> m_isRunning;            ///< 捕获状态
    std::atomic<int> m_pendingFrames{0};      ///< 主线程待处理帧数
    cv::Mat m_currentFrame;                   ///< 当前捕获的帧
    std::mutex m_frameMutex;                  ///< 保护当前帧的互斥锁
    const int m_frameIntervalMs = 33;         ///< 目标帧间隔（约30FPS）
};

} // namespace Core
} // namespace ArcticOwl

#endif // ARCTICOWL_CORE_VIDEO_CAPTURE_H