#pragma once
#include <QWidget>
#include "ui_ShirakanaUI.h"
#include <string>
#include <vector>
#include <cmath>

//using namespace cv;
//using namespace cv::dnn;
using std::string;
using std::vector;



class ShirakanaUI : public QWidget
{
	Q_OBJECT

public:
	ShirakanaUI(QWidget *parent = nullptr);
	~ShirakanaUI();
	void loadMods();
	void cleanInputs();

private slots:
	void OnShirakanaModelInsertOfficialClick();
	void OnShirakanaStartClick();


private:
	Ui::ShirakanaUI ui;
	FILE* torchModel = nullptr;
	vector<string> modList;
	vector<string> modNames;
	vector<string> modModerType;
	QString thisModFile;
	int curModIndex;
};
