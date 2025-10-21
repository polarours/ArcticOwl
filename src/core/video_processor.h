#ifndef ARCTICOWL_CORE_VIDEO_PROCESSOR_H
#define ARCTICOWL_CORE_VIDEO_PROCESSOR_H

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

namespace ArcticOwl {
namespace Core {

class VideoProcessor {
public:
    struct DetectionResult {
        enum Type {
            INTRUSION,
            FIRE,
            EQUIPMENT_FAILURE,
            MOTION
        };

        Type type;
        cv::Rect boundingBox;
        float confidence;
        std::string description;
    };

    VideoProcessor();
    ~VideoProcessor();

    std::vector<DetectionResult> processFrame(const cv::Mat& frame);
    void setIntrusionDetection(bool enabled) { m_intrusionDetection = enabled; }
    void setFireDetection(bool enabled) { m_fireDetection = enabled; }
    void setMotionDetection(bool enabled) { m_motionDetection = enabled; }

private:
    std::vector<DetectionResult> detectMotion(const cv::Mat& frame);
    std::vector<DetectionResult> detectIntrusion(const cv::Mat& frame);
    std::vector<DetectionResult> detectFire(const cv::Mat& frame);
    double calculateTextureFeature(const cv::Mat& image);
    double calculateShapeFeature(const std::vector<cv::Point>& contour);
    void updateAccumulatedBackground(const cv::Mat& frame);

    bool m_intrusionDetection;
    bool m_fireDetection;
    bool m_motionDetection;

    cv::Ptr<cv::BackgroundSubtractor> m_backgroundSubtractorMOG2;
    cv::Ptr<cv::BackgroundSubtractor> m_backgroundSubtractorKNN;

    cv::Mat m_accumulatedBackground;
    int m_frameCount;

};

} // namespace Core
} // namespace ArcticOwl

#endif // ARCTICOWL_CORE_VIDEO_PROCESSOR_H