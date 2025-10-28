#include <iostream>
#include <algorithm>
#include <cmath>

#include "video_processor.h"

namespace ArcticOwl::Core {

VideoProcessor::VideoProcessor()
    : m_intrusionDetection(true)
    , m_fireDetection(true)
    , m_motionDetection(true)
    , m_frameCount(0)
{
    m_backgroundSubtractorMOG2 = cv::createBackgroundSubtractorMOG2(500, 16, true);
    m_backgroundSubtractorKNN = cv::createBackgroundSubtractorKNN(500, 400, true);
}

VideoProcessor::~VideoProcessor()
{
    m_backgroundSubtractorMOG2.release();
    m_backgroundSubtractorKNN.release();
}

std::vector<VideoProcessor::DetectionResult> VideoProcessor::processFrame(const cv::Mat& frame)
{
    std::vector<DetectionResult> results;

    if (frame.empty()) {
        return results;
    }

    try {
        updateAccumulatedBackground(frame);

        if (m_motionDetection) {
            auto motionResults = detectMotion(frame);
            results.insert(results.end(), motionResults.begin(), motionResults.end());
        }

        if (m_intrusionDetection) {
            auto intrusionResults = detectIntrusion(frame);
            results.insert(results.end(), intrusionResults.begin(), intrusionResults.end());
        }

        if (m_fireDetection) {
            auto fireResults = detectFire(frame);
            results.insert(results.end(), fireResults.begin(), fireResults.end());
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error while processing frame: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error while processing frame: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while processing frame" << std::endl;
    }

    return results;
}

std::vector<VideoProcessor::DetectionResult> VideoProcessor::detectMotion(const cv::Mat& frame)
{
    std::vector<DetectionResult> results;

    if (frame.empty()) {
        return results;
    }

    try {
        cv::Mat fgMaskMOG2, fgMaskKNN;
        m_backgroundSubtractorMOG2->apply(frame, fgMaskMOG2);
        m_backgroundSubtractorKNN->apply(frame, fgMaskKNN);

        cv::Mat combinedMask;
        cv::bitwise_and(fgMaskMOG2, fgMaskKNN, combinedMask);

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(combinedMask, combinedMask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(combinedMask, combinedMask, cv::MORPH_CLOSE, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(combinedMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < 500) continue;

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
        std::cerr << "OpenCV error during motion detection: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during motion detection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error during motion detection" << std::endl;
    }

    return results;
}

std::vector<VideoProcessor::DetectionResult> VideoProcessor::detectIntrusion(const cv::Mat& frame)
{
    std::vector<DetectionResult> results;

    if (frame.empty()) {
        return results;
    }

    try {
        cv::Rect intrusionArea(frame.cols / 4, frame.rows / 4, frame.cols / 2, frame.rows / 2);

        auto motionResults = detectMotion(frame);

        for (const auto& motion : motionResults) {
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
        std::cerr << "OpenCV error during intrusion detection: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during intrusion detection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error during intrusion detection" << std::endl;
    }

    return results;
}

std::vector<VideoProcessor::DetectionResult> VideoProcessor::detectFire(const cv::Mat& frame)
{
    std::vector<DetectionResult> results;

    if (frame.empty()) {
        return results;
    }

    try {
        cv::Mat hsv, yuv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
        cv::cvtColor(frame, yuv, cv::COLOR_BGR2YUV);

        cv::Mat color_mask_fire_hsv_lower, color_mask_fire_hsv_upper, color_mask_fire_hsv;
        cv::inRange(hsv, cv::Scalar(0, 100, 100), cv::Scalar(15, 255, 255), color_mask_fire_hsv_lower);
        cv::inRange(hsv, cv::Scalar(160, 100, 100), cv::Scalar(180, 255, 255), color_mask_fire_hsv_upper);
        cv::bitwise_or(color_mask_fire_hsv_lower, color_mask_fire_hsv_upper, color_mask_fire_hsv);

        std::vector<cv::Mat> yuv_channels;
        cv::split(yuv, yuv_channels);
        cv::Mat y_channel = yuv_channels[0];
        cv::Mat u_channel = yuv_channels[1];

        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(color_mask_fire_hsv, color_mask_fire_hsv, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(color_mask_fire_hsv, color_mask_fire_hsv, cv::MORPH_CLOSE, kernel);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(color_mask_fire_hsv, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < 500) continue;
            cv::Rect boundingBox = cv::boundingRect(contour);

            boundingBox.x = std::max(0, boundingBox.x);
            boundingBox.y = std::max(0, boundingBox.y);
            boundingBox.width = std::min(frame.cols - boundingBox.x, boundingBox.width);
            boundingBox.height = std::min(frame.rows - boundingBox.y, boundingBox.height);

            if (boundingBox.width > 0 && boundingBox.height > 0) {
                cv::Mat region = frame(boundingBox);

                double texture = calculateTextureFeature(region);

                double shape = calculateShapeFeature(contour);

                double colorConfidence = std::min(1.0, area / 5000.0);
                double textureConfidence = texture;
                double shapeConfidence = shape;
                double confidence = (colorConfidence + textureConfidence + shapeConfidence) / 3.0;

                if (confidence > 0.4) {
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
        std::cerr << "OpenCV error during fire detection: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during fire detection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error during fire detection" << std::endl;
    }

    return results;
}

double VideoProcessor::calculateTextureFeature(const cv::Mat& image)
{
    if (image.empty()) {
        return 0.0;
    }

    try {
        cv::Mat gray;
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

        double meanGradient = cv::mean(magnitude)[0];

        return std::max(0.0, std::min(1.0, meanGradient / 255.0));
    } catch (...) {
        return 0.0;
    }
}

double VideoProcessor::calculateShapeFeature(const std::vector<cv::Point>& contour)
{
    if (contour.empty()) {
        return 0.0;
    }

    try {
        double area = cv::contourArea(contour);
        double perimeter = cv::arcLength(contour, true);

        if (perimeter == 0) return 0;

        double circularity = (4 * CV_PI * area) / (perimeter * perimeter);

        return std::max(0.0, std::min(1.0, circularity / 0.8));
    } catch (...) {
        return 0.0;
    }
}

void VideoProcessor::updateAccumulatedBackground(const cv::Mat& frame) {
    if (frame.empty()) {
        return;
    }

    try {
        if (m_accumulatedBackground.empty()) {
            m_accumulatedBackground = cv::Mat::zeros(frame.size(), CV_32FC3);
            m_frameCount = 0;
        }

        cv::Mat floatFrame;
        frame.convertTo(floatFrame, CV_32FC3);

        cv::accumulate(floatFrame, m_accumulatedBackground);
        m_frameCount++;

        if (m_frameCount >= 100) {
            m_accumulatedBackground /= static_cast<float>(m_frameCount);
            m_accumulatedBackground.convertTo(m_accumulatedBackground, CV_8UC3);
            m_frameCount = 0;
        }
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error while updating background: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error while updating background: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while updating background" << std::endl;
    }
}

}