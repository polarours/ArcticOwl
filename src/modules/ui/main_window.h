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
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QTranslator>
#include <opencv2/opencv.hpp>

#include "core/video_capture.h"
#include "core/video_processor.h"

namespace ArcticOwl::Modules::Network {
class NetworkServer;
}

namespace ArcticOwl::Modules::UI {

// Main application window class.
// This class manages the UI components and interactions.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void startSystem();
    void stopSystem();

    void updateFrame(const cv::Mat& frame);
    void updateAlerts();

    void onIntrusionDetectionChanged(bool enabled);
    void onFireDetectionChanged(bool enabled);
    void onMotionDetectionChanged(bool enabled);

    void openSettingsDialog();
    void showAboutDialog();

private:
    enum class Language {
        English,
        Chinese
    };

    void initializeSystem();
    void cleanupSystem();

    void setupUI();
    void setupMenus();
    void setupLanguageMenu();

    void retranslateUi();
    void updateActionChecks();
    bool loadLanguage(Language language);

    void changeEvent(QEvent* event) override;

    bool m_systemRunning;
    bool m_hasEverStarted;
    QLabel* m_videoLabel;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QCheckBox* m_intrusionCheckBox;
    QCheckBox* m_fireCheckBox;
    QCheckBox* m_motionCheckBox;
    QTextEdit* m_alertsLog;
    QTableWidget* m_camerasTable;
    QSpinBox* m_cameraIdSpinBox;
    QTimer* m_alertsTimer;
    QComboBox* m_cameraSourceCombo;
    QGroupBox* m_controlGroup;
    QGroupBox* m_cameraGroup;
    QGroupBox* m_detectionGroup;
    QGroupBox* m_alertsGroup;
    QLabel* m_cameraIdLabel;
    QLabel* m_videoSourceLabel;
    QLabel* m_camerasTableLabel;

    QMenu* m_settingsMenu;
    QMenu* m_helpMenu;
    QMenu* m_languageMenu;
    QAction* m_preferencesAction;
    QAction* m_aboutAction;
    QAction* m_languageEnglishAction;
    QAction* m_languageChineseAction;
    QActionGroup* m_languageActionGroup;

    int m_networkPort = 8080;
    int m_alertIntervalMs = 1000;

    Language m_currentLanguage = Language::English;
    QTranslator m_translator;

    Core::VideoCapture* m_videoCapture;
    Core::VideoProcessor* m_videoProcessor;
    Network::NetworkServer* m_networkServer;
};

} // namespace ArcticOwl::Modules::UI

#endif // ARCTICOWL_MODULES_UI_MAIN_WINDOW_H