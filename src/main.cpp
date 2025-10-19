#include <iostream>
#include <opencv2/opencv.hpp>
#include <QtWidgets/QApplication>

#include "modules/ui/main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("ArcticOwl");

    ArcticOwl::Modules::UI::MainWindow window;
    window.show();

    return app.exec();
}