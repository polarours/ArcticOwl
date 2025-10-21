#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QSpinBox>
#include <QtGui/QPixmap>
#include <QtGui/QImage>
#include <QtGui/QCloseEvent>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <opencv2/opencv.hpp>

#include "modules/ui/main_window.h"
#include "core/video_capture.h"
#include "core/video_processor.h"
#include "modules/network/network_server.h"
#include "arctic_owl/version.h"

namespace ArcticOwl {
namespace Modules {
namespace UI {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_systemRunning(false)
    , m_hasEverStarted(false)
    , m_videoLabel(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_intrusionCheckBox(nullptr)
    , m_fireCheckBox(nullptr)
    , m_motionCheckBox(nullptr)
    , m_alertsLog(nullptr)
    , m_camerasTable(nullptr)
    , m_cameraIdSpinBox(nullptr)
    , m_alertsTimer(nullptr)
    , m_cameraSourceCombo(nullptr)
    , m_controlGroup(nullptr)
    , m_cameraGroup(nullptr)
    , m_detectionGroup(nullptr)
    , m_alertsGroup(nullptr)
    , m_cameraIdLabel(nullptr)
    , m_videoSourceLabel(nullptr)
    , m_camerasTableLabel(nullptr)
    , m_settingsMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_languageMenu(nullptr)
    , m_preferencesAction(nullptr)
    , m_aboutAction(nullptr)
    , m_languageEnglishAction(nullptr)
    , m_languageChineseAction(nullptr)
    , m_languageActionGroup(nullptr)
    , m_videoCapture(nullptr)
    , m_videoProcessor(nullptr)
    , m_networkServer(nullptr)
{
    setupUI();

    m_alertsTimer = new QTimer(this);
    m_alertsTimer->setInterval(m_alertIntervalMs);
    connect(m_alertsTimer, &QTimer::timeout, this, &MainWindow::updateAlerts);

    if (m_videoCapture) {
        connect(m_videoCapture, &Core::VideoCapture::frameReady,
                this, &MainWindow::updateFrame, Qt::QueuedConnection);
    }

    retranslateUi();
    updateActionChecks();
}

MainWindow::~MainWindow()
{
    stopSystem();
}

void MainWindow::setupUI()
{
    resize(1200,  800);

    setupMenus();

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto* mainLayout = new QVBoxLayout(centralWidget);

    m_videoLabel = new QLabel(this);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("QLabel { background-color : black; color : white; }");
    m_videoLabel->setMinimumSize(800, 600);
    mainLayout->addWidget(m_videoLabel);

    auto* buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(this);
    m_stopButton = new QPushButton(this);
    m_stopButton->setEnabled(false);

    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    m_controlGroup = new QGroupBox(this);
    auto* controlLayout = new QHBoxLayout(m_controlGroup);

    m_cameraGroup = new QGroupBox(this);
    auto* cameraLayout = new QVBoxLayout(m_cameraGroup);

    auto* idLayout = new QHBoxLayout();
    m_cameraIdLabel = new QLabel(this);
    idLayout->addWidget(m_cameraIdLabel);
    m_cameraIdSpinBox = new QSpinBox(this);
    m_cameraIdSpinBox->setRange(0, 10);
    m_cameraIdSpinBox->setValue(0);
    idLayout->addWidget(m_cameraIdSpinBox);
    cameraLayout->addLayout(idLayout);

    auto* sourceLayout = new QHBoxLayout();
    m_videoSourceLabel = new QLabel(this);
    sourceLayout->addWidget(m_videoSourceLabel);
    m_cameraSourceCombo = new QComboBox(this);
    m_cameraSourceCombo->addItem(QString());
    m_cameraSourceCombo->addItem(QString());
    m_cameraSourceCombo->addItem(QString());
    sourceLayout->addWidget(m_cameraSourceCombo);
    cameraLayout->addLayout(sourceLayout);

    controlLayout->addWidget(m_cameraGroup);

    m_detectionGroup = new QGroupBox(this);
    auto* detectionLayout = new QVBoxLayout(m_detectionGroup);

    m_intrusionCheckBox = new QCheckBox(this);
    m_intrusionCheckBox->setChecked(true);

    m_fireCheckBox = new QCheckBox(this);
    m_fireCheckBox->setChecked(true);

    m_motionCheckBox = new QCheckBox(this);
    m_motionCheckBox->setChecked(true);

    detectionLayout->addWidget(m_intrusionCheckBox);
    detectionLayout->addWidget(m_fireCheckBox);
    detectionLayout->addWidget(m_motionCheckBox);

    controlLayout->addWidget(m_detectionGroup);

    connect(m_intrusionCheckBox, &QCheckBox::toggled,
            this, &MainWindow::onIntrusionDetectionChanged);
    connect(m_fireCheckBox, &QCheckBox::toggled,
            this, &MainWindow::onFireDetectionChanged);
    connect(m_motionCheckBox, &QCheckBox::toggled,
            this, &MainWindow::onMotionDetectionChanged);

    controlLayout->addStretch();
    mainLayout->addWidget(m_controlGroup);

    m_alertsGroup = new QGroupBox(this);
    auto* alertsLayout = new QVBoxLayout(m_alertsGroup);

    m_alertsLog = new QTextEdit(this);
    m_alertsLog->setReadOnly(true);
    m_alertsLog->setMaximumHeight(150);

    alertsLayout->addWidget(m_alertsLog);
    mainLayout->addWidget(m_alertsGroup);

    m_camerasTableLabel = new QLabel(this);
    mainLayout->addWidget(m_camerasTableLabel);

    m_camerasTable = new QTableWidget(0, 3, this);
    m_camerasTable->setHorizontalHeaderLabels({QString(), QString(), QString()});
    mainLayout->addWidget(m_camerasTable);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::startSystem);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopSystem);
}

void MainWindow::retranslateUi()
{
    const QString version = QString::fromLatin1(ArcticOwl::Version::kString);
    setWindowTitle(tr("ArcticOwl v%1").arg(version));

    const bool hasPixmap = m_videoLabel && !m_videoLabel->pixmap().isNull();

    if (m_videoLabel) {
        if (m_systemRunning) {
            if (!hasPixmap) {
                m_videoLabel->setText(tr("Acquiring video stream..."));
            }
        } else {
            if (hasPixmap) {
                m_videoLabel->clear();
            } else if (m_hasEverStarted) {
                m_videoLabel->setText(tr("System stopped."));
            } else {
                m_videoLabel->setText(tr("Press \"Start System\" to begin monitoring."));
            }
        }
    }

    if (m_startButton) {
        m_startButton->setText(tr("Start System"));
    }
    if (m_stopButton) {
        m_stopButton->setText(tr("Stop System"));
    }

    if (m_controlGroup) {
        m_controlGroup->setTitle(tr("System Control"));
    }
    if (m_cameraGroup) {
        m_cameraGroup->setTitle(tr("Camera Settings"));
    }
    if (m_detectionGroup) {
        m_detectionGroup->setTitle(tr("Detection Settings"));
    }
    if (m_alertsGroup) {
        m_alertsGroup->setTitle(tr("Security Alerts"));
    }

    if (m_cameraIdLabel) {
        m_cameraIdLabel->setText(tr("Camera ID:"));
    }
    if (m_videoSourceLabel) {
        m_videoSourceLabel->setText(tr("Video Source:"));
    }

    if (m_cameraSourceCombo) {
        m_cameraSourceCombo->setItemText(0, tr("Local Camera"));
        m_cameraSourceCombo->setItemText(1, tr("RTSP Stream"));
        m_cameraSourceCombo->setItemText(2, tr("RTMP Stream"));
    }

    if (m_intrusionCheckBox) {
        m_intrusionCheckBox->setText(tr("Intrusion Detection"));
    }
    if (m_fireCheckBox) {
        m_fireCheckBox->setText(tr("Fire Detection"));
    }
    if (m_motionCheckBox) {
        m_motionCheckBox->setText(tr("Motion Detection"));
    }

    if (m_camerasTableLabel) {
        m_camerasTableLabel->setText(tr("Camera List:"));
    }
    if (m_camerasTable) {
        m_camerasTable->setHorizontalHeaderLabels({
            tr("ID"),
            tr("Location"),
            tr("Status")});
    }

    if (m_settingsMenu) {
        m_settingsMenu->setTitle(tr("Settings"));
    }
    if (m_preferencesAction) {
        m_preferencesAction->setText(tr("Preferences..."));
    }
    if (m_languageMenu) {
        m_languageMenu->setTitle(tr("Language"));
    }
    if (m_languageEnglishAction) {
        m_languageEnglishAction->setText(tr("English"));
    }
    if (m_languageChineseAction) {
        m_languageChineseAction->setText(tr("Chinese"));
    }
    if (m_helpMenu) {
        m_helpMenu->setTitle(tr("Help"));
    }
    if (m_aboutAction) {
        m_aboutAction->setText(tr("About ArcticOwl"));
    }

    updateActionChecks();
}

void MainWindow::updateActionChecks()
{
    if (m_languageEnglishAction) {
        m_languageEnglishAction->setChecked(m_currentLanguage == Language::English);
    }
    if (m_languageChineseAction) {
        m_languageChineseAction->setChecked(m_currentLanguage == Language::Chinese);
    }
}

bool MainWindow::loadLanguage(Language language)
{
    if (m_currentLanguage == language && language == Language::English) {
        return true;
    }

    qApp->removeTranslator(&m_translator);

    bool success = true;

    if (language == Language::Chinese) {
        const QString resourcePath = QStringLiteral(":/i18n/arcticowl_zh_CN.qm");
        success = m_translator.load(resourcePath);
        if (success) {
            qApp->installTranslator(&m_translator);
        } else {
            QMessageBox::warning(this, tr("Warning"), tr("Chinese translation file is missing."));
            language = Language::English;
        }
    }

    m_currentLanguage = language;
    retranslateUi();
    return success;
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);

    if (!event) {
        return;
    }

    if (event->type() == QEvent::LanguageChange || event->type() == QEvent::LocaleChange) {
        retranslateUi();
    }
}

void MainWindow::setupMenus()
{
    m_settingsMenu = menuBar()->addMenu(QString());
    m_preferencesAction = m_settingsMenu->addAction(QString());
    connect(m_preferencesAction, &QAction::triggered, this, &MainWindow::openSettingsDialog);

    setupLanguageMenu();

    m_helpMenu = menuBar()->addMenu(QString());
    m_aboutAction = m_helpMenu->addAction(QString());
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::setupLanguageMenu()
{
    m_languageMenu = m_settingsMenu->addMenu(QString());

    m_languageActionGroup = new QActionGroup(this);
    m_languageActionGroup->setExclusive(true);

    m_languageEnglishAction = m_languageMenu->addAction(QString());
    m_languageEnglishAction->setCheckable(true);

    m_languageChineseAction = m_languageMenu->addAction(QString());
    m_languageChineseAction->setCheckable(true);

    m_languageActionGroup->addAction(m_languageEnglishAction);
    m_languageActionGroup->addAction(m_languageChineseAction);

    connect(m_languageEnglishAction, &QAction::triggered, this, [this]() {
        loadLanguage(Language::English);
    });

    connect(m_languageChineseAction, &QAction::triggered, this, [this]() {
        loadLanguage(Language::Chinese);
    });
}

void MainWindow::openSettingsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Preferences"));
    dialog.setModal(true);

    auto* layout = new QFormLayout(&dialog);

    auto* portSpin = new QSpinBox(&dialog);
    portSpin->setRange(1024, 65535);
    portSpin->setValue(m_networkPort);
    layout->addRow(tr("Network Port:"), portSpin);

    auto* intervalSpin = new QSpinBox(&dialog);
    intervalSpin->setRange(200, 10000);
    intervalSpin->setSingleStep(100);
    intervalSpin->setValue(m_alertIntervalMs);
    layout->addRow(tr("Alert Refresh Interval (ms):"), intervalSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        const bool portChanged = (m_networkPort != portSpin->value());
        m_networkPort = portSpin->value();
        m_alertIntervalMs = intervalSpin->value();
        m_alertsTimer->setInterval(m_alertIntervalMs);

        if (m_systemRunning && portChanged) {
            QMessageBox::information(this,
                                     tr("Settings Updated"),
                                     tr("The network port will take effect the next time the system starts."));
        }
    }
}

void MainWindow::showAboutDialog()
{
    const QString aboutText = tr(
        "ArcticOwl v%1\n\n"
        "Real-time video monitoring and alerting demo.\n"
        "Built with Qt 6, OpenCV, and Boost.Asio.\n"
        "Project site: github.com/polarours/ArcticOwl")
        .arg(QString::fromLatin1(ArcticOwl::Version::kString));

    QMessageBox::about(this, tr("About ArcticOwl"), aboutText);
}

void MainWindow::initializeSystem()
{
    try {
        int sourceIndex = m_cameraSourceCombo->currentIndex();
        if (sourceIndex == 0) {
            m_videoCapture = new Core::VideoCapture(nullptr, m_cameraIdSpinBox->value());
        } else if (sourceIndex == 1) {
            QString rtspUrl = QInputDialog::getText(this,
                                                   tr("RTSP Stream URL"),
                                                   tr("Enter the RTSP stream URL:"));
            if (rtspUrl.isEmpty()) {
                throw std::runtime_error(tr("RTSP stream URL cannot be empty.").toUtf8().toStdString());
            }
            m_videoCapture = new Core::VideoCapture(nullptr, -1, rtspUrl.toStdString());
        } else if (sourceIndex == 2) {
            QString rtmpUrl = QInputDialog::getText(this,
                                                   tr("RTMP Stream URL"),
                                                   tr("Enter the RTMP stream URL:"));
            if (rtmpUrl.isEmpty()) {
                throw std::runtime_error(tr("RTMP stream URL cannot be empty.").toUtf8().toStdString());
            }
            m_videoCapture = new Core::VideoCapture(nullptr, -1, "", rtmpUrl.toStdString());
        }

        m_videoProcessor = new Core::VideoProcessor();
        m_networkServer = new Network::NetworkServer(m_networkPort);

        if (m_videoProcessor) {
            m_videoProcessor->setIntrusionDetection(m_intrusionCheckBox->isChecked());
            m_videoProcessor->setFireDetection(m_fireCheckBox->isChecked());
            m_videoProcessor->setMotionDetection(m_motionCheckBox->isChecked());
        }

        if (m_videoCapture) {
            connect(m_videoCapture, &Core::VideoCapture::frameReady,
                    this, &MainWindow::updateFrame, Qt::QueuedConnection);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize system: " << e.what() << std::endl;
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("Failed to initialize the system: %1").arg(QString::fromUtf8(e.what())));
        throw;
    } catch (...) {
        std::cerr << "Unexpected error while initializing system" << std::endl;
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("An unexpected error occurred while initializing the system."));
        throw;
    }
}

void MainWindow::cleanupSystem()
{
    try {
        if (m_videoCapture) {
            m_videoCapture->stopVideoCaptureSystem();
            delete m_videoCapture;
            m_videoCapture = nullptr;
        }

        if (m_videoProcessor) {
            delete m_videoProcessor;
            m_videoProcessor = nullptr;
        }

        if (m_networkServer) {
            m_networkServer->stopNetworkSystem();
            delete m_networkServer;
            m_networkServer = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to clean up system: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while cleaning up system" << std::endl;
    }
}

void MainWindow::startSystem()
{
    if (m_systemRunning) return;

    try {
        initializeSystem();

        if (m_videoCapture && !m_videoCapture->startVideoCaptureSystem()) {
            QMessageBox::critical(this,
                                  tr("Error"),
                                  tr("Failed to start the camera."));
            cleanupSystem();
            return;
        }

        if (m_networkServer) {
            m_networkServer->startNetworkSystem();
        }

        m_systemRunning = true;
        m_hasEverStarted = true;
        m_startButton->setEnabled(false);
        m_stopButton->setEnabled(true);

        m_alertsTimer->start(m_alertIntervalMs);

        if (m_videoLabel->pixmap().isNull()) {
            m_videoLabel->setText(tr("Acquiring video stream..."));
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to start system: " << e.what() << std::endl;
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("Failed to start the system: %1").arg(QString::fromUtf8(e.what())));

        cleanupSystem();
        m_systemRunning = false;
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
    } catch (...) {
        std::cerr << "Unexpected error while starting system" << std::endl;
        QMessageBox::critical(this,
                              tr("Error"),
                              tr("An unexpected error occurred while starting the system."));
        
        cleanupSystem();
        m_systemRunning = false;
        m_startButton->setEnabled(true);
        m_stopButton->setEnabled(false);
    }
}

/**
 * Stops capture, processing, and networking services, then releases resources.
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
        m_videoLabel->clear();
        m_videoLabel->setText(tr("System stopped."));
    } catch (const std::exception& e) {
        std::cerr << "Failed to stop system: " << e.what() << std::endl;
        QMessageBox::warning(this,
                             tr("Warning"),
                             tr("Failed to stop the system: %1").arg(QString::fromUtf8(e.what())));
    } catch (...) {
        std::cerr << "Unexpected error while stopping system" << std::endl;
        QMessageBox::warning(this,
                             tr("Warning"),
                             tr("An unexpected error occurred while stopping the system."));
    }
}

void MainWindow::updateFrame(const cv::Mat& frame)
{
    try {
        if (frame.empty()) {
            return;
        }

        cv::Mat processedFrame = frame.clone();

        if (m_videoProcessor) {
            auto results = m_videoProcessor->processFrame(frame);
            for (const auto& r : results) {
                cv::Scalar color(0, 255, 0);
                if (r.type == Core::VideoProcessor::DetectionResult::FIRE) color = cv::Scalar(0, 0, 255);
                if (r.type == Core::VideoProcessor::DetectionResult::INTRUSION) color = cv::Scalar(255, 0, 0);
                cv::rectangle(processedFrame, r.boundingBox, color, 2);
                std::string label = r.description + " (" + std::to_string(r.confidence) + ")";
                cv::Point textOrg(r.boundingBox.x, std::max(0, r.boundingBox.y - 5));
                cv::putText(processedFrame, label, textOrg, cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
            }
        }

        if (m_networkServer) {
            m_networkServer->broadcastFrame(processedFrame);
        }

        cv::Mat rgbFrame;
        if (processedFrame.channels() == 3) {
            cv::cvtColor(processedFrame, rgbFrame, cv::COLOR_BGR2RGB);
        } else {
            cv::cvtColor(processedFrame, rgbFrame, cv::COLOR_GRAY2RGB);
        }
        QImage qimg(rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
                    static_cast<int>(rgbFrame.step), QImage::Format_RGB888);

        m_videoLabel->setPixmap(QPixmap::fromImage(qimg).scaled(
            m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV error: " << e.what() << std::endl;
        QMetaObject::invokeMethod(this, "stopSystem", Qt::QueuedConnection);
    } catch (const std::exception& e) {
        std::cerr << "Failed to update frame: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while updating frame" << std::endl;
    }
}

void MainWindow::updateAlerts()
{
    try {
        static int alertCount = 0;
        if (m_systemRunning && (alertCount % 10 == 0) && m_alertsLog) {
            const QString statusTemplate = tr("[%1] System running normally");
            const QString alertMsg = statusTemplate.arg(QTime::currentTime().toString());
            m_alertsLog->append(alertMsg);
        }
        alertCount++;
    } catch (const std::exception& e) {
        std::cerr << "Failed to update alerts: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while updating alerts" << std::endl;
    }
}

void MainWindow::onIntrusionDetectionChanged(bool enabled)
{
    try {
        if (m_videoProcessor) {
            m_videoProcessor->setIntrusionDetection(enabled);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to toggle intrusion detection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while toggling intrusion detection" << std::endl;
    }
}

void MainWindow::onFireDetectionChanged(bool enabled)
{
    try {
        if (m_videoProcessor) {
            m_videoProcessor->setFireDetection(enabled);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to toggle fire detection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while toggling fire detection" << std::endl;
    }
}

void MainWindow::onMotionDetectionChanged(bool enabled)
{
    try {
        if (m_videoProcessor) {
            m_videoProcessor->setMotionDetection(enabled);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to toggle motion detection: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unexpected error while toggling motion detection" << std::endl;
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_systemRunning) {
        stopSystem();
    }

    event->accept();
    QMainWindow::closeEvent(event);
}

} // namespace UI
} // namespace Modules
} // namespace ArcticOwl