#include <chrono>
#include <thread>

#include "video_capture.h"

namespace ArcticOwl {
namespace Core {

/**
 * @brief 构造函数
 *
 * @param parent 父对象
 * @param camera_id 摄像头ID，默认-1表示不使用本地摄像头
 * @param rtsp_url RTSP流URL，默认空字符串表示不使用RTSP流
 * @param rtmp_url RTMP流URL，默认空字符串表示不使用RTMP流
 */
VideoCapture::VideoCapture(QObject* parent, int camera_id, const std::string& rtsp_url, const std::string& rtmp_url)
    : QObject(parent)
    , m_cameraId(camera_id)
    , m_rtspUrl(rtsp_url)
    , m_rtmpUrl(rtmp_url)
    , m_isRunning(false)
{
    // 可以选择在这里初始化其他组件
    // m_processor = new VideoProcessor();
    // m_networkServer = new NetworkServer(8080);
    // ...

}

/**
 * @brief 析构函数
 */
VideoCapture::~VideoCapture()
{
    // 确保在销毁对象前停止视频捕获系统
    stopVideoCaptureSystem();
}

/**
 * @brief 启动视频捕获系统
 *
 * @return true 如果成功启动
 * @return false 如果启动失败
 */
bool VideoCapture::startVideoCaptureSystem() {
    try {
        // 根据提供的参数打开摄像头或RTSP/RTMP流
        if (!m_rtspUrl.empty()) {
            m_capture.open(m_rtspUrl); // 打开RTSP流
        } else if (!m_rtmpUrl.empty()) {
            m_capture.open(m_rtmpUrl); // 打开RTMP流
        } else {
            m_capture.open(m_cameraId); // 打开摄像头
        }

        // 检查视频捕获是否成功打开
        if (!m_capture.isOpened()){
            std::cerr << "无法打开摄像头或RTSP/RTMP流，请检查摄像头是否连接或RTSP/RTMP流URL是否正确。摄像头ID： " << m_cameraId
                       << "，RTSP流URL： " << m_rtspUrl << "，RTMP流URL： "
                       << m_rtmpUrl << std::endl;
            return false;
        }

        // 将缓冲区长度设为1，以降低网络流引入的延迟
        m_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);

        m_isRunning = true;
        m_captureThread = std::thread(&VideoCapture::captureLoop, this); // 运行捕获线程

        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV 错误：" << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "未知错误。" << std::endl;
        return false;
    }
}

/**
 * @brief 停止视频捕获系统
 *
 * @return true 如果成功停止
 * @return false 如果停止失败
 */
bool VideoCapture::stopVideoCaptureSystem() {
    try {
        m_isRunning = false;

        // 等待捕获线程结束
        if (m_captureThread.joinable()) {
            m_captureThread.join();
        }

        // 释放视频捕获资源
        if (m_capture.isOpened()) {
            m_capture.release();
        }

        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV 错误：" << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
    } catch (...) {
        std::cerr << "未知错误。" << std::endl;
    }

    return false;
}

/**
 * @brief 获取当前捕获的帧
 *
 * @return cv::Mat 当前帧的副本，如果没有帧则返回空矩阵
 */
cv::Mat VideoCapture::getCurrentFrame() {
    try {
        std::lock_guard<std::mutex> lock(m_frameMutex); // 保护当前帧
        if (m_currentFrame.empty()) {
            return cv::Mat(); // 返回空矩阵
        }
        return m_currentFrame.clone();
    } catch (const std::exception& e) {
        std::cerr << "获取当前帧时发生错误: " << e.what() << std::endl;
        return cv::Mat();
    } catch (...) {
        std::cerr << "获取当前帧时发生未知错误" << std::endl;
        return cv::Mat();
    }
}

/**
 * @brief 检查视频捕获是否已打开
 *
 * @return true 如果视频捕获已打开
 * @return false 如果视频捕获未打开或发生错误
 */
bool VideoCapture::isOpened() const {
    try {
        return m_capture.isOpened();
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV 错误：" << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "未知错误。" << std::endl;
        return false;
    }
}

/**
 * @brief 捕获视频帧的线程函数
 * @details 该函数在独立线程中运行，持续捕获视频帧并通过信号发送新帧
 */
void VideoCapture::captureLoop() {
    const auto interval = std::chrono::milliseconds(m_frameIntervalMs); // 目标帧间隔（约30FPS）

    // 捕获循环
    while (m_isRunning) {
        const auto loopStart = std::chrono::steady_clock::now(); // 循环开始时间

        try {
            cv::Mat frame;
            bool frame_read = m_capture.read(frame);

            // 如果成功读取帧，则处理并发送信号
            if (frame_read && !frame.empty()) {
                std::lock_guard<std::mutex> lock(m_frameMutex);
                m_currentFrame = frame.clone();

                // 通过信号将新帧发送给主线程
                if (m_pendingFrames.load(std::memory_order_relaxed) < 2) {
                    m_pendingFrames.fetch_add(1, std::memory_order_relaxed);
                    QMetaObject::invokeMethod(this, [this, frame]() {
                            emit frameReady(frame);
                            m_pendingFrames.fetch_sub(1, std::memory_order_relaxed);
                    }, Qt::QueuedConnection);
                }
            } else if (!frame_read) {
                std::cerr << "无法读取视频帧，请检查摄像头连接或RTSP/RTMP流URL。" << std::endl;
                std::this_thread::sleep_for(interval);
            }
        } catch (const cv::Exception& e) {
            std::cerr << "OpenCV 错误：" << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            std::cerr << "错误：" << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (...) {
            std::cerr << "未知错误。" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        const auto loopEnd = std::chrono::steady_clock::now(); // 循环结束时间
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(loopEnd - loopStart); // 计算循环耗时
        
        // 控制帧率
        if (elapsed < interval) {
            std::this_thread::sleep_for(interval - elapsed);
        }
    }
}

} // namespace Core
} // namespace ArcticOwl