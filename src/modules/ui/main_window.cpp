#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QCloseEvent>
#include <QtCore/QThread>
#include <opencv2/opencv.hpp>

#include "modules/ui/main_window.h"
#include "core/video_capture.h"
#include "core/video_processor.h"
#include "modules/network/network_server.h"

namespace ArcticOwl {
namespace Modules {
namespace UI {

/**
 * @brief 构造函数
 * @details 初始化UI组件和成员变量
 *
 * @param parent 父对象
 */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_videoCapture(nullptr)
    , m_videoProcessor(nullptr)
    , m_networkServer(nullptr)
    , m_systemRunning(false)
{
    // 设置UI
    setupUI();

    // 设置警报更新定时器
    m_alertsTimer = new QTimer(this);
    m_alertsTimer->setInterval(1000); // 每秒更新一次
    // 连接定时器信号到更新警报槽
    connect(m_alertsTimer, &QTimer::timeout, this, &MainWindow::updateAlerts);

    // 设置处理帧的队列连接，确保线程安全
    if (m_videoCapture) {
        connect(m_videoCapture, &Core::VideoCapture::frameReady,
                this, &MainWindow::updateFrame, Qt::QueuedConnection);
    }
}

/**
 * @brief 析构函数
 * @details 停止系统并释放资源
 */
MainWindow::~MainWindow()
{
    stopSystem();
}

/**
 * @brief 设置UI组件
 * @details 创建并布局所有UI元素，包括视频显示、控制按钮和警报面板
 */
void MainWindow::setupUI()
{
    // 设置窗口属性
    setWindowTitle("ArcticOwl");
    resize(1200,  800);

    // 创建中央部件
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // 视频显示区域
    m_videoLabel = new QLabel("点击\"启动系统\"开始监控", this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("QLabel { background-color : black; color : white; }");
    m_videoLabel->setMinimumSize(800, 600);
    mainLayout->addWidget(m_videoLabel);

    // 控制按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton("启动系统", this);
    m_stopButton = new QPushButton("停止系统", this);
    m_stopButton->setEnabled(false);

    // 添加按钮到布局
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    // 创建控制面板
    QGroupBox* controlGroup = new QGroupBox("系统控制", this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);

    // 摄像头控制
    QGroupBox* cameraGroup = new QGroupBox("摄像头设置", this);
    QVBoxLayout* cameraLayout = new QVBoxLayout(cameraGroup);

    // 摄像头ID输入
    QHBoxLayout* idLayout = new QHBoxLayout();
    idLayout->addWidget(new QLabel("摄像头ID:", this));
    m_cameraIdSpinBox = new QSpinBox(this);
    m_cameraIdSpinBox->setRange(0, 10);
    m_cameraIdSpinBox->setValue(0);
    idLayout->addWidget(m_cameraIdSpinBox);
    cameraLayout->addLayout(idLayout);

    // 摄像头源选择
    QHBoxLayout* sourceLayout = new QHBoxLayout();
    sourceLayout->addWidget(new QLabel("视频源:", this));
    m_cameraSourceCombo = new QComboBox(this);
    m_cameraSourceCombo->addItem("本地摄像头"); // 默认选项
    m_cameraSourceCombo->addItem("RTSP流"); // 添加RTSP选项
    m_cameraSourceCombo->addItem("RTMP流"); // 添加RTMP选项
    sourceLayout->addWidget(m_cameraSourceCombo);
    cameraLayout->addLayout(sourceLayout);

    controlLayout->addWidget(cameraGroup);

    // 检测控制
    QGroupBox* detectionGroup = new QGroupBox("检测设置", this);
    QVBoxLayout* detectionLayout = new QVBoxLayout(detectionGroup);

    // 入侵检测复选框
    m_intrusionCheckBox = new QCheckBox("入侵检测", this);
    m_intrusionCheckBox->setChecked(true);

    // 火焰检测复选框
    m_fireCheckBox = new QCheckBox("火焰检测", this);
    m_fireCheckBox->setChecked(true);

    // 运动检测复选框
    m_motionCheckBox = new QCheckBox("运动检测", this);
    m_motionCheckBox->setChecked(true);

    // 添加复选框到布局
    detectionLayout->addWidget(m_intrusionCheckBox);
    detectionLayout->addWidget(m_fireCheckBox);
    detectionLayout->addWidget(m_motionCheckBox);

    controlLayout->addWidget(detectionGroup);

    // 连接检测控制信号
    connect(m_intrusionCheckBox, &QCheckBox::toggled,
            this, &MainWindow::onIntrusionDetectionChanged);
    connect(m_fireCheckBox, &QCheckBox::toggled,
            this, &MainWindow::onFireDetectionChanged);
    connect(m_motionCheckBox, &QCheckBox::toggled,
            this, &MainWindow::onMotionDetectionChanged);

    controlLayout->addStretch();
    mainLayout->addWidget(controlGroup);

    // 警报面板
    QGroupBox* alertsGroup = new QGroupBox("安全警报", this);
    QVBoxLayout* alertsLayout = new QVBoxLayout(alertsGroup);

    // 警报日志文本编辑器
    m_alertsLog = new QTextEdit(this);
    m_alertsLog->setReadOnly(true);
    m_alertsLog->setMaximumHeight(150);

    alertsLayout->addWidget(m_alertsLog);
    mainLayout->addWidget(alertsGroup);

    // 摄像头列表
    m_camerasTable = new QTableWidget(0, 3, this);
    m_camerasTable->setHorizontalHeaderLabels({"ID", "位置", "状态"});
    mainLayout->addWidget(new QLabel("摄像头列表:", this));
    mainLayout->addWidget(m_camerasTable);

    // 连接信号和槽
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startSystem);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopSystem);
}

/**
 * @brief 初始化系统组件
 * @details 创建视频捕获、处理和网络服务对象，设置检测选项
 */
void MainWindow::initializeSystem()
{
    try {
        // 根据选择的视频源创建视频捕获对象
        int sourceIndex = m_cameraSourceCombo->currentIndex();
        if (sourceIndex == 0) {
            // 本地摄像头
            m_videoCapture = new Core::VideoCapture(nullptr, m_cameraIdSpinBox->value());
        } else if (sourceIndex == 1) {
            // RTSP流
            QString rtspUrl = QInputDialog::getText(this, "RTSP流地址", "请输入RTSP流地址:");
            if (rtspUrl.isEmpty()) {
                throw std::runtime_error("RTSP流地址不能为空");
            }
            m_videoCapture = new Core::VideoCapture(nullptr, -1, rtspUrl.toStdString());
        } else if (sourceIndex == 2) {
            // RTMP流
            QString rtmpUrl = QInputDialog::getText(this, "RTMP流地址", "请输入RTMP流地址:");
            if (rtmpUrl.isEmpty()) {
                throw std::runtime_error("RTMP流地址不能为空");
            }
            m_videoCapture = new Core::VideoCapture(nullptr, -1, "", rtmpUrl.toStdString());
        }

        // 初始化视频处理器
        m_videoProcessor = new Core::VideoProcessor();
        // 初始化网络服务器，监听端口8080
        m_networkServer = new Network::NetworkServer(8080);
        // 设置警报更新定时器
        m_alertsTimer->setInterval(1000); // 每秒更新一次警报

        // 设置检测选项，默认全部启用
        if (m_videoProcessor) {
            m_videoProcessor->setIntrusionDetection(m_intrusionCheckBox->isChecked());
            m_videoProcessor->setFireDetection(m_fireCheckBox->isChecked());
            m_videoProcessor->setMotionDetection(m_motionCheckBox->isChecked());
        }

        // 连接视频帧信号，使用队列连接确保线程安全
        if (m_videoCapture) {
            connect(m_videoCapture, &Core::VideoCapture::frameReady,
                    this, &MainWindow::updateFrame, Qt::QueuedConnection);
        }
    } catch (const std::exception& e) {
        std::cerr << "初始化系统时发生错误: " << e.what() << std::endl;
        QMessageBox::critical(this, "错误", QString("初始化系统时发生错误: %1").arg(e.what()));
        throw;
    } catch (...) {
        std::cerr << "初始化系统时发生未知错误" << std::endl;
        QMessageBox::critical(this, "错误", "初始化系统时发生未知错误");
        throw;
    }
}

/**
 * @brief 清理系统资源
 * @details 停止视频捕获、处理和网络服务，释放资源
 */
void MainWindow::cleanupSystem()
{
    try {
        // 停止视频采集
        if (m_videoCapture) {
            m_videoCapture->stopVideoCaptureSystem();
            delete m_videoCapture;
            m_videoCapture = nullptr;
        }

        // 清理视频处理器
        if (m_videoProcessor) {
            delete m_videoProcessor;
            m_videoProcessor = nullptr;
        }

        // 停止网络服务器
        if (m_networkServer) {
            m_networkServer->stopNetworkSystem();
            delete m_networkServer;
            m_networkServer = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "清理系统时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "清理系统时发生未知错误" << std::endl;
    }
}

/**
 * @brief 启动系统
 * @details 初始化并启动视频捕获、处理和网络服务
 */
void MainWindow::startSystem()
{
    if (m_systemRunning) return;

    try {
        initializeSystem();

        // 启动视频捕获系统
        if (m_videoCapture && !m_videoCapture->startVideoCaptureSystem()) {
            QMessageBox::critical(this, "错误", "无法启动摄像头");
            cleanupSystem();
            return;
        }

        // 启动网络服务器
        if (m_networkServer) {
            m_networkServer->startNetworkSystem();
        }

        // 更新UI状态
        m_systemRunning = true;
        m_startButton->setEnabled(false);
        m_stopButton->setEnabled(true);

        // 启动警报定时器
        m_alertsTimer->start(1000); // 每秒更新一次警报

        m_videoLabel->setText("正在获取视频流...");
    } catch (const std::exception& e) {
        std::cerr << "启动系统时发生错误: " << e.what() << std::endl;
        QMessageBox::critical(this, "错误", QString("启动系统时发生错误: %1").arg(e.what()));
        
        cleanupSystem();
        m_systemRunning = false;
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
    } catch (...) {
        std::cerr << "启动系统时发生未知错误" << std::endl;
        QMessageBox::critical(this, "错误", "启动系统时发生未知错误");
        
        cleanupSystem();
        m_systemRunning = false;
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
    }
}

/**
 * @brief 停止系统
 * @details 停止视频捕获、处理和网络服务，清理资源
 */
void MainWindow::stopSystem()
{
    if (!m_systemRunning) {
        return;
    }

    try {
        m_alertsTimer->stop();
        cleanupSystem();
        
        m_systemRunning = false;
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        m_videoLabel->setText("系统已停止");
    } catch (const std::exception& e) {
        std::cerr << "停止系统时发生错误: " << e.what() << std::endl;
        QMessageBox::warning(this, "警告", QString("停止系统时发生错误: %1").arg(e.what()));
    } catch (...) {
        std::cerr << "停止系统时发生未知错误" << std::endl;
        QMessageBox::warning(this, "警告", "停止系统时发生未知错误");
    }
}

/**
 * @brief 关闭事件处理
 * @details 在窗口关闭时确保系统正确停止
 *
 * @param event 关闭事件
 */
void MainWindow::updateFrame(const cv::Mat& frame)
{
    try {
        // 检查输入帧是否有效
        if (frame.empty()) {
            return;
        }

        // 使用拷贝以便绘制检测结果
        cv::Mat processedFrame = frame.clone();

        // 进行检测并在图像上绘制结果
        if (m_videoProcessor) {
            auto results = m_videoProcessor->processFrame(frame);
            for (const auto& r : results) {
                // 选择颜色基于检测类型
                cv::Scalar color(0, 255, 0); // 默认绿
                if (r.type == Core::VideoProcessor::DetectionResult::FIRE) color = cv::Scalar(0, 0, 255);
                if (r.type == Core::VideoProcessor::DetectionResult::INTRUSION) color = cv::Scalar(255, 0, 0);
                // 绘制边界框
                cv::rectangle(processedFrame, r.boundingBox, color, 2);
                // 绘制标签和置信度
                std::string label = r.description + " (" + std::to_string(r.confidence) + ")";
                int baseline = 0;
                cv::Size textSize __attribute__((unused)) = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
                cv::Point textOrg(r.boundingBox.x, std::max(0, r.boundingBox.y - 5));
                cv::putText(processedFrame, label, textOrg, cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
            }
        }

        // 将处理后的帧发送到网络服务器
        if (m_networkServer) {
            m_networkServer->broadcastFrame(processedFrame);
        }

        // 将 cv::Mat 转换为 QImage 并显示
        cv::Mat rgbFrame;
        if (processedFrame.channels() == 3) {
            cv::cvtColor(processedFrame, rgbFrame, cv::COLOR_BGR2RGB);
        } else {
            cv::cvtColor(processedFrame, rgbFrame, cv::COLOR_GRAY2RGB);
        }
        // 转换为 QImage
        QImage qimg(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
                    static_cast<int>(rgbFrame.step), QImage::Format_RGB888);

        // 在标签中显示图像，保持纵横比
        m_videoLabel->setPixmap(QPixmap::fromImage(qimg).scaled(
            m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV错误: " << e.what() << std::endl;
        // 遇到OpenCV错误时停止系统
        QMetaObject::invokeMethod(this, "stopSystem", Qt::QueuedConnection);
    } catch (const std::exception& e) {
        std::cerr << "更新视频帧时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "更新视频帧时发生未知错误" << std::endl;
    }
}

/**
 * @brief 更新警报信息
 * @details 定期检查系统状态并更新警报日志
 */
void MainWindow::updateAlerts()
{
    try {
        // 更新警报日志，这里只是模拟一些警报信息
        static int alertCount = 0;
        // 模拟警报更新逻辑
        if (m_systemRunning && (alertCount % 10 == 0)) {
            QString alertMsg = QString("[%1] 系统运行正常").arg(QTime::currentTime().toString());
            m_alertsLog->append(alertMsg);
        }
        alertCount++;
    } catch (const std::exception& e) {
        std::cerr << "更新警报时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "更新警报时发生未知错误" << std::endl;
    }
}

/**
 * @brief 入侵检测选项更改处理
 * @details 启用或禁用入侵检测功能
 *
 * @param enabled 是否启用入侵检测
 */
void MainWindow::onIntrusionDetectionChanged(bool enabled)
{
    try {
        // 设置入侵检测选项
        if (m_videoProcessor) {
            m_videoProcessor->setIntrusionDetection(enabled);
        }
    } catch (const std::exception& e) {
        std::cerr << "设置入侵检测时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "设置入侵检测时发生未知错误" << std::endl;
    }
}

/**
 * @brief 火焰检测选项更改处理
 * @details 启用或禁用火焰检测功能
 *
 * @param enabled 是否启用火焰检测
 */
void MainWindow::onFireDetectionChanged(bool enabled)
{
    try {
        // 设置火焰检测选项
        if (m_videoProcessor) {
            m_videoProcessor->setFireDetection(enabled);
        }
    } catch (const std::exception& e) {
        std::cerr << "设置火焰检测时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "设置火焰检测时发生未知错误" << std::endl;
    }
}

/**
 * @brief 运动检测选项更改处理
 * @details 启用或禁用运动检测功能
 *
 * @param enabled 是否启用运动检测
 */
void MainWindow::onMotionDetectionChanged(bool enabled)
{
    try {
        // 设置运动检测选项
        if (m_videoProcessor) {
            m_videoProcessor->setMotionDetection(enabled);
        }
    } catch (const std::exception& e) {
        std::cerr << "设置运动检测时发生错误: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "设置运动检测时发生未知错误" << std::endl;
    }
}

/**
 * @brief 关闭事件处理
 * @details 在窗口关闭时确保系统已停止并清理资源
 *
 * @param event 关闭事件
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
    // 在窗口关闭时确保系统已停止并清理资源
    if (m_systemRunning) {
        stopSystem();
    }

    // 接受关闭事件并转发给基类处理
    event->accept();
    QMainWindow::closeEvent(event);
}

} // namespace UI
} // namespace Modules
} // namespace ArcticOwl