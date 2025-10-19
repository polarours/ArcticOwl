#include <iostream>
#include <algorithm>
#include <cmath>

#include "video_processor.h"
#include "modules/ui/main_window.h"

namespace ArcticOwl {
namespace Core {

/**
 * @brief 构造函数
 * @details 初始化检测选项、背景减除器和其他成员变量
 */
VideoProcessor::VideoProcessor()
    : m_intrusionDetection(true)
    , m_fireDetection(true)
    , m_motionDetection(true)
    , m_frameCount(0)
{
    // 初始化背景减除器
    m_backgroundSubtractorMOG2 = cv::createBackgroundSubtractorMOG2(500, 16, true);
    m_backgroundSubtractorKNN = cv::createBackgroundSubtractorKNN(500, 400, true);
}

/**
 * @brief 析构函数
 * @details 释放背景减除器资源
 */
VideoProcessor::~VideoProcessor()
{
    // 释放背景减除器资源
    m_backgroundSubtractorMOG2.release();
    m_backgroundSubtractorKNN.release();
}

/**
 * @brief 更新累积背景帧
 * @details 使用当前帧更新背景模型
 *
 * @param frame 当前视频帧
 */
std::vector<VideoProcessor::DetectionResult> VideoProcessor::processFrame(const cv::Mat& frame)
{
    std::vector<DetectionResult> results; // 存储所有检测结果

    if (frame.empty()) {
        return results;
    }

    try {
        // 更新累积背景
        updateAccumulatedBackground(frame);

        // 运动检测
        if (m_motionDetection) {
            auto motionResults = detectMotion(frame);
            results.insert(results.end(), motionResults.begin(), motionResults.end());
        }

        // 入侵检测
        if (m_intrusionDetection) {
            auto intrusionResults = detectIntrusion(frame);
            results.insert(results.end(), intrusionResults.begin(), intrusionResults.end());
        }

        // 火焰检测
        if (m_fireDetection) {
            auto fireResults = detectFire(frame);
            results.insert(results.end(), fireResults.begin(), fireResults.end());
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV错误在视频处理时: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "视频处理时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "视频处理时发生未知错误" << std::endl;
    }

    return results;
}

/**
 * @brief 更新累积背景帧
 * @details 使用当前帧更新背景模型
 *
 * @param frame 当前视频帧
 */
std::vector<VideoProcessor::DetectionResult> VideoProcessor::detectMotion(const cv::Mat& frame)
{
    std::vector<DetectionResult> results; // 存储运动检测结果

    // 检查输入帧是否有效
    if (frame.empty()) {
        return results;
    }

    try {
        // 应用背景减除算法
        cv::Mat fgMaskMOG2, fgMaskKNN;
        m_backgroundSubtractorMOG2->apply(frame, fgMaskMOG2);
        m_backgroundSubtractorKNN->apply(frame, fgMaskKNN);

        // 结合两种掩码
        cv::Mat combinedMask;
        cv::bitwise_and(fgMaskMOG2, fgMaskKNN, combinedMask);

        // 形态学操作去噪
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(combinedMask, combinedMask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(combinedMask, combinedMask, cv::MORPH_CLOSE, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(combinedMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 过滤区域并生成检测结果
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < 500) continue; // 过滤小区域

            cv::Rect boundingBox = cv::boundingRect(contour);
            double confidence = std::min(1.0, area / 10000.0);

            DetectionResult result;
            result.type = DetectionResult::MOTION;
            result.boundingBox = boundingBox;
            result.confidence = static_cast<float>(confidence);
            result.description = "motion";
            results.push_back(result);
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV错误在运动检测中: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "运动检测时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "运动检测时发生未知错误" << std::endl;
    }

    return results;
}

/**
 * @brief 入侵检测
 * @details 基于预定义的入侵区域和运动检测结果进行入侵检测
 *
 * @param frame 当前视频帧
 * @return std::vector<DetectionResult> 检测结果列表
 */
std::vector<VideoProcessor::DetectionResult> VideoProcessor::detectIntrusion(const cv::Mat& frame)
{
    std::vector<DetectionResult> results; // 存储入侵检测结果

    // 检查输入帧是否有效
    if (frame.empty()) {
        return results;
    }

    try {
        // 定义入侵区域（示例：图像中心区域）
        cv::Rect intrusionArea(frame.cols / 4, frame.rows / 4, frame.cols / 2, frame.rows / 2);

        // 使用运动检测结果作为入侵检测的基础
        auto motionResults = detectMotion(frame);

        for (const auto& motion : motionResults) {
            // 检查运动边界框是否与入侵区域有重叠
            if ((motion.boundingBox & intrusionArea).area() > 0) {
                DetectionResult result;
                result.type = DetectionResult::INTRUSION;
                result.boundingBox = motion.boundingBox;
                result.confidence = motion.confidence; // 使用运动检测的置信度
                result.description = "intrusion";
                results.push_back(result);
            }
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV错误在入侵检测中: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "入侵检测时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "入侵检测时发生未知错误" << std::endl;
    }

    return results;
}

/**
 * @brief 火焰检测
 * @details 基于颜色、纹理和形状特征进行火焰检测
 *
 * @param frame 当前视频帧
 * @return std::vector<DetectionResult> 检测结果列表
 */
std::vector<VideoProcessor::DetectionResult> VideoProcessor::detectFire(const cv::Mat& frame)
{
    std::vector<DetectionResult> results; // 存储火焰检测结果

    // 检查输入帧是否有效
    if (frame.empty()) {
        return results;
    }

    try {
        cv::Mat hsv, yuv;
        // 转换到HSV和YUV颜色空间
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
        cv::cvtColor(frame, yuv, cv::COLOR_BGR2YUV);

        // HSV颜色空间中的火焰范围
        cv::Mat color_mask_fire_hsv_lower, color_mask_fire_hsv_upper, color_mask_fire_hsv;
        cv::inRange(hsv, cv::Scalar(0, 100, 100), cv::Scalar(15, 255, 255), color_mask_fire_hsv_lower);
        cv::inRange(hsv, cv::Scalar(160, 100, 100), cv::Scalar(180, 255, 255), color_mask_fire_hsv_upper);
        cv::bitwise_or(color_mask_fire_hsv_lower, color_mask_fire_hsv_upper, color_mask_fire_hsv);

        // YUV颜色空间中的火焰范围（Y通道和U通道）
        std::vector<cv::Mat> yuv_channels;
        cv::split(yuv, yuv_channels);
        cv::Mat y_channel = yuv_channels[0];
        cv::Mat u_channel = yuv_channels[1];

        // 形态学操作去噪
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(color_mask_fire_hsv, color_mask_fire_hsv, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(color_mask_fire_hsv, color_mask_fire_hsv, cv::MORPH_CLOSE, kernel);

        // 查找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(color_mask_fire_hsv, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 过滤区域并计算多种特征
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < 500) continue; // 过滤小区域
            cv::Rect boundingBox = cv::boundingRect(contour);

            // 确保边界框在图像范围内
            boundingBox.x = std::max(0, boundingBox.x);
            boundingBox.y = std::max(0, boundingBox.y);
            boundingBox.width = std::min(frame.cols - boundingBox.x, boundingBox.width);
            boundingBox.height = std::min(frame.rows - boundingBox.y, boundingBox.height);

            if (boundingBox.width > 0 && boundingBox.height > 0) {
                cv::Mat region = frame(boundingBox);

                // 计算纹理特征
                double texture = calculateTextureFeature(region);

                // 计算形状特征
                double shape = calculateShapeFeature(contour);

                // 综合置信度
                double colorConfidence = std::min(1.0, area / 5000.0);
                double textureConfidence = texture; // 纹理特征已在0-1之间
                double shapeConfidence = shape;     // 形状特征已在0-1之间
                double confidence = (colorConfidence + textureConfidence + shapeConfidence) / 3.0;

                if (confidence > 0.4) { // 综合置信度阈值
                    DetectionResult result;
                    result.type = DetectionResult::FIRE;
                    result.boundingBox = boundingBox;
                    result.confidence = static_cast<float>(std::min(1.0, confidence));
                    result.description = "fire";
                    results.push_back(result);
                }
            }
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV错误在火焰检测中: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "火焰检测时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "火焰检测时发生未知错误" << std::endl;
    }

    return results;
}

/**
 * @brief 计算纹理特征
 * @details 使用梯度幅值的平均值作为纹理特征
 *
 * @param image 输入图像区域
 * @return double 归一化的纹理特征值（0-1之间）
 */
double VideoProcessor::calculateTextureFeature(const cv::Mat& image)
{
    // 检查输入图像是否有效
    if (image.empty()) {
        return 0.0;
    }

    try {
        cv::Mat gray;
        // 转换为灰度图
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = image;
        }

        // 计算梯度
        cv::Mat grad_x, grad_y;
        cv::Sobel(gray, grad_x, CV_64F, 1, 0, 3);
        cv::Sobel(gray, grad_y, CV_64F, 0, 1, 3);

        cv::Mat magnitude;
        cv::magnitude(grad_x, grad_y, magnitude);

        // 计算平均梯度幅值作为纹理特征
        double meanGradient = cv::mean(magnitude)[0];

        // 归一化到0-1之间（假设最大可能值为255）
        return std::max(0.0, std::min(1.0, meanGradient / 255.0));
    } catch (...) {
        // 出现任何错误时返回默认值
        return 0.0;
    }
}

/**
 * @brief 计算形状特征
 * @details 使用轮廓的圆形度作为形状特征
 *
 * @param contour 输入轮廓点
 * @return double 归一化的形状特征值（0-1之间）
 */
double VideoProcessor::calculateShapeFeature(const std::vector<cv::Point>& contour)
{
    // 检查轮廓是否有效
    if (contour.empty()) {
        return 0.0;
    }

    try {
        double area = cv::contourArea(contour); // 计算面积
        double perimeter = cv::arcLength(contour, true); // 计算周长

        if (perimeter == 0) return 0;

        // 计算圆形度（0-1之间，1表示完美的圆）
        double circularity = (4 * CV_PI * area) / (perimeter * perimeter);

        // 火焰通常不是完美的圆形，有一个合理的范围
        return std::max(0.0, std::min(1.0, circularity / 0.8));
    } catch (...) {
        // 出现任何错误时返回默认值
        return 0.0;
    }
}

/**
 * @brief 更新累积背景帧
 * @details 使用当前帧更新背景模型
 *
 * @param frame 当前视频帧
 */
void VideoProcessor::updateAccumulatedBackground(const cv::Mat& frame) {
    // 检查输入帧是否有效
    if (frame.empty()) {
        return;
    }

    try {
        // 初始化累积背景
        if (m_accumulatedBackground.empty()) {
            m_accumulatedBackground = cv::Mat::zeros(frame.size(), CV_32FC3);
            m_frameCount = 0;
        }

        // 将当前帧转换为浮点型
        cv::Mat floatFrame;
        frame.convertTo(floatFrame, CV_32FC3);

        // 累积背景
        cv::accumulate(floatFrame, m_accumulatedBackground);
        m_frameCount++;

        // 每100帧更新一次背景
        if (m_frameCount >= 100) {
            m_accumulatedBackground /= static_cast<float>(m_frameCount);
            m_accumulatedBackground.convertTo(m_accumulatedBackground, CV_8UC3);
            m_frameCount = 0;
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV错误在更新累积背景时: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "更新累积背景时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "更新累积背景时发生未知错误" << std::endl;
    }
}

} // namespace Core
} // namespace ArcticOwl