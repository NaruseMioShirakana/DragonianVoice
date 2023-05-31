#include "MainWindow/moevoicestudio.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString& locale : uiLanguages) {
        const QString baseName = "MoeVS_" + QLocale(locale).name();
        if (translator.load(QString(":/%1").arg(baseName))) {
            app.installTranslator(&translator);
            break;
        }
    }
    MoeVoiceStudio DWindow;
    DWindow.show();
    return app.exec();
}
