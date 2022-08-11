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
	std::wstring getCleanerStr(std::wstring input);

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
	vector<std::wstring> modList;
	vector<std::wstring> modNames;
	vector<std::wstring> modModerType;
	vector<std::wstring> modSymbol;
	vector<std::wstring> modCleaners;
	vector<vector<std::wstring>> modSpeakers;
	QString thisModFile;
	int curModIndex;
	QMediaPlayer *player;
	QAudioOutput *audioOutput;
	bool isPlay = false;

};
