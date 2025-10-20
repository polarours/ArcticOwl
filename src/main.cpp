#include <iostream>
#include <QString>
#include <opencv2/opencv.hpp>
#include <QtWidgets/QApplication>

#include "modules/ui/main_window.h"
#include "arctic_owl/version.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("ArcticOwl");
    app.setApplicationVersion(QString::fromLatin1(ArcticOwl::Version::kString));
    app.setApplicationDisplayName(QStringLiteral("ArcticOwl v%1").arg(QString::fromLatin1(ArcticOwl::Version::kString)));

    ArcticOwl::Modules::UI::MainWindow window;
    window.show();

    return app.exec();
}