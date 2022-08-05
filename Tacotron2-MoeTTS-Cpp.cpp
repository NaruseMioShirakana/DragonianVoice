#include <QApplication>
#include "ShirakanaUI.h"
#pragma execution_character_set("utf-8")
int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	ShirakanaUI shirakanaMainWindow;
	shirakanaMainWindow.show();
	return app.exec();
}