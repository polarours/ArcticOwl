#ifndef ARCTICOWL_MODULES_UI_MAIN_WINDOW_H
#define ARCTICOWL_MODULES_UI_MAIN_WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QTextEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QTime>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCloseEvent>
#include <QInputDialog>
#include <opencv2/opencv.hpp>

#include "core/video_capture.h"
#include "core/video_processor.h"

// 前向声明 NetworkServer
namespace ArcticOwl {
namespace Modules {
namespace Network {
class NetworkServer;
} // namespace Network
} // namespace Modules
} // namespace ArcticOwl

namespace ArcticOwl {
namespace Modules  {
namespace UI {

/**
 * @brief The MainWindow class
 * @details 负责UI显示和用户交互，管理视频捕获、处理和网络通信
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 构造函数和析构函数
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    // 关闭事件处理
    void closeEvent(QCloseEvent* event) override;

private slots:
    // 开启系统
    void startSystem();
    // 关闭系统
    void stopSystem();

    // 更新视频帧
    void updateFrame(const cv::Mat& frame);
    // 更新警报信息
    void updateAlerts();

    // 入侵检测开关变化处理
    void onIntrusionDetectionChanged(bool enabled);
    // 火焰检测开关变化处理
    void onFireDetectionChanged(bool enabled);
    // 运动检测开关变化处理
    void onMotionDetectionChanged(bool enabled);

private:
    // 系统初始化
    void initializeSystem();
    // 系统清理
    void cleanupSystem();

    // UI设置
    void setupUI();

    // 设置警报面板
    void setupAlertPanel();
    // 设置摄像头列表
    void setupCameraList();
    // 设置摄像头控制
    void setupCameraControls();
    // 设置检测控制
    void setupDetectionControls();

    bool m_systemRunning;                    ///< 系统运行状态
    QLabel* m_videoLabel;                    ///< 视频显示标签
    QPushButton* m_startButton;              ///< 启动按钮
    QPushButton* m_stopButton;               ///< 停止按钮
    QCheckBox* m_intrusionCheckBox;          ///< 入侵检测复选框
    QCheckBox* m_fireCheckBox;               ///< 火焰检测复选框
    QCheckBox* m_motionCheckBox;             ///< 运动检测复选框
    QTextEdit* m_alertsLog;                  ///< 警报日志文本编辑器
    QTableWidget* m_camerasTable;            ///< 摄像头列表表格
    QSpinBox* m_cameraIdSpinBox;             ///< 摄像头ID输入框
    QTimer* m_alertsTimer;                   ///< 警报更新定时器
    QComboBox* m_cameraSourceCombo;          ///< 摄像头源选择下拉框

    Core::VideoCapture* m_videoCapture;      ///< 视频捕获对象
    Core::VideoProcessor* m_videoProcessor;  ///< 视频处理对象
    Network::NetworkServer* m_networkServer; ///< 网络服务器对象
};

} // namespace UI
} // namespace Modules
} // namespace ArcticOwl

#endif // ARCTICOWL_MODULES_UI_MAIN_WINDOW_H