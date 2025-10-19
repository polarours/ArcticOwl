#ifndef ARCTICOWL_CORE_VIDEO_PROCESSOR_H
#define ARCTICOWL_CORE_VIDEO_PROCESSOR_H

#include <vector>
#include <opencv2/opencv.hpp>

namespace ArcticOwl {
namespace Core {

/**
 * @brief The VideoProcessor class
 * @details 负责处理视频帧，执行运动检测、入侵检测和火焰检测
 */
class VideoProcessor {
public:
    /**
     * @brief 检测结果结构体
     * @details 用于存储检测结果的信息，包括类型、边界框、置信度和描述
     */
    struct DetectionResult {
        enum Type {
            INTRUSION,         ///< 入侵检测
            FIRE,              ///< 火焰检测
            EQUIPMENT_FAILURE, ///< 设备故障检测
            MOTION             ///< 运动检测
        };

        Type type;               ///< 检测类型
        cv::Rect boundingBox;    ///< 检测边界框
        float confidence;        ///< 置信度
        std::string description; ///< 描述信息
    };

    // 构造函数
    VideoProcessor();
    // 析构函数
    ~VideoProcessor();

    // 处理视频帧并返回检测结果
    std::vector<DetectionResult> processFrame(const cv::Mat& frame);
    // 设置入侵检测开关
    void setIntrusionDetection(bool enabled) { m_intrusionDetection = enabled; }
    // 设置火焰检测开关
    void setFireDetection(bool enabled) { m_fireDetection = enabled; }
    // 设置运动检测开关
    void setMotionDetection(bool enabled) { m_motionDetection = enabled; }

private:
    // 运动检测
    std::vector<DetectionResult> detectMotion(const cv::Mat& frame);
    // 入侵检测
    std::vector<DetectionResult> detectIntrusion(const cv::Mat& frame);
    // 火焰检测
    std::vector<DetectionResult> detectFire(const cv::Mat& frame);
    // 计算纹理特征
    double calculateTextureFeature(const cv::Mat& image);
    // 计算形状特征
    double calculateShapeFeature(const std::vector<cv::Point>& contour);
    // 更新累积背景帧
    void updateAccumulatedBackground(const cv::Mat& frame);

    // 检测选项
    bool m_intrusionDetection;  ///< 入侵检测开关
    bool m_fireDetection;       ///< 火焰检测开关
    bool m_motionDetection;     ///< 运动检测开关

    // 背景减除器用于运动检测
    cv::Ptr<cv::BackgroundSubtractor> m_backgroundSubtractorMOG2; ///< MOG2背景减除器
    cv::Ptr<cv::BackgroundSubtractor> m_backgroundSubtractorKNN;  ///< KNN背景减除器

    // 用于累积背景帧
    cv::Mat m_accumulatedBackground; ///< 累积背景帧
    int m_frameCount;                ///< 累积帧计数

};

} // namespace Core
} // namespace ArcticOwl

#endif // ARCTICOWL_CORE_VIDEO_PROCESSOR_H