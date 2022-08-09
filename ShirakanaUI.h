#pragma once
#include <QWidget>
#include "ui_ShirakanaUI.h"
#include <string>
#include <vector>
#include <cmath>
#include <QMediaPlayer>
#include <QAudioOutput>
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
	void OnWavSaveClick();
	void onSliderVolumeValueChanged(int value);
	void onSliderMusicTimeValueChanged(qint64 value);
	void seekChange(int position);
	void OnPlayerButtonClick();
	void getDuration(qint64 time);

private:
	Ui::ShirakanaUI ui;
	FILE* torchModel = nullptr;
	vector<string> modList;
	vector<string> modNames;
	vector<string> modModerType;
	vector<string> modSymbol;
	QString thisModFile;
	int curModIndex;
	QMediaPlayer *player;
	QAudioOutput *audioOutput;
	bool isPlay = false;

};
