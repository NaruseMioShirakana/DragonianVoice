#include "ShirakanaUI.h"
#include <string>
#include <QMessageBox>
#include <QFileDialog>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <QElapsedTimer>
#include <fstream>
using namespace std;
const int HEAD_LENGTH = 2 * 1024 * 1024;//2MB
class Wav {
public:
	Wav() {
		header = {
			{'R','I','F','F'},
			0,
			{'W','A','V','E'},
			{'f','m','t',' '},
			16,
			1,
			1,
			22050,
			22050 * 2,
			2,
			16,
			{'d','a','t','a'},
			0
		};
		StartPos = 44;
		Data = nullptr;
	}
	Wav(const char* Path) {
		char* buf = (char*)malloc(sizeof(char) * HEAD_LENGTH);
		FILE* stream;
		freopen_s(&stream, Path, "rb", stderr);
		if (stream == nullptr || buf == NULL) {
			*this = Wav();
			return;
		}
		fread(buf, 1, HEAD_LENGTH, stream);
		int pos = 0;
		while (pos < HEAD_LENGTH) {
			if ((buf[pos] == 'R') && (buf[pos + 1] == 'I') && (buf[pos + 2] == 'F') & (buf[pos + 3] == 'F')) {
				header.RIFF[0] = 'R';
				header.RIFF[1] = 'I';
				header.RIFF[2] = 'F';
				header.RIFF[3] = 'F';
				pos += 4;
				break;
			}
			++pos;
		}
		header.ChunkSize = *(int*)&buf[pos];
		pos += 4;
		header.WAVE[0] = buf[pos];
		header.WAVE[1] = buf[pos + 1];
		header.WAVE[2] = buf[pos + 2];
		header.WAVE[3] = buf[pos + 3];
		pos += 4;
		while (pos < HEAD_LENGTH) {
			if ((buf[pos] == 'f') && (buf[pos + 1] == 'm') && (buf[pos + 2] == 't')) {
				header.fmt[0] = 'f';
				header.fmt[1] = 'm';
				header.fmt[2] = 't';
				header.fmt[3] = ' ';
				pos += 4;
				break;
			}
			++pos;
		}
		header.Subchunk1Size = *(int*)&buf[pos];
		pos += 4;
		header.AudioFormat = *(short*)&buf[pos];
		pos += 2;
		header.NumOfChan = *(short*)&buf[pos];
		pos += 2;
		header.SamplesPerSec = *(int*)&buf[pos];
		pos += 4;
		header.bytesPerSec = *(int*)&buf[pos];
		pos += 4;
		header.blockAlign = *(short*)&buf[pos];
		pos += 2;
		header.bitsPerSample = *(short*)&buf[pos];
		pos += 2;
		while (pos < HEAD_LENGTH) {
			if ((buf[pos] == 'd') && (buf[pos + 1] == 'a') && (buf[pos + 2] == 't') & (buf[pos + 3] == 'a')) {
				header.Subchunk2ID[0] = 'd';
				header.Subchunk2ID[1] = 'a';
				header.Subchunk2ID[2] = 't';
				header.Subchunk2ID[3] = 'a';
				pos += 4;
				break;
			}
			++pos;
		}
		header.Subchunk2Size = *(int*)&buf[pos];
		pos += 4;
		StartPos = pos;
		Data = (char*)malloc(sizeof(char) * header.Subchunk2Size + 1);
		if (Data == NULL) {
			*this = Wav();
			return;
		}
		memcpy(Data, &buf[StartPos], header.Subchunk2Size);
		free(buf);
		fclose(stream);
	}
	~Wav() {}

	bool isEmpty() {
		return this->header.Subchunk2Size == 0;
	}
	Wav catWav(Wav input) {
		char* buffer = (char*)malloc(sizeof(char) * this->header.Subchunk2Size + 1);
		if (buffer == NULL)return *this;
		memcpy(buffer, this->Data, header.Subchunk2Size);
		free(this->Data);
		this->Data = (char*)malloc(sizeof(char) * ((int64_t)this->header.Subchunk2Size + (int64_t)input.header.Subchunk2Size)+ 1);
		if (this->Data == NULL) {
			throw exception("RAM不足");
		}
		memcpy(this->Data, buffer, header.Subchunk2Size);
		memcpy(this->Data+ header.Subchunk2Size, input.Data, input.header.Subchunk2Size);
		free(buffer);
		this->header.ChunkSize += input.header.Subchunk2Size;
		this->header.Subchunk2Size += input.header.Subchunk2Size;
		return *this;
	}
	void write(const char* filepath) {
		std::ofstream ocout;
		char* outputData = Data;
		ocout.open(filepath, std::ios::out | std::ios::binary);
		ocout.write((char*)&header, 44);
		ocout.write(outputData, (int32_t)(this->header.Subchunk2Size));
		ocout.close();
	}
	string getInformation() {
		string buffer = "";
		buffer += header.RIFF[0];
		buffer += header.RIFF[1];
		buffer += header.RIFF[2];
		buffer += header.RIFF[3];
		buffer += "\n";
		buffer += to_string(header.ChunkSize);
		buffer += "\n";
		buffer += header.WAVE[0];
		buffer += header.WAVE[1];
		buffer += header.WAVE[2];
		buffer += header.WAVE[3];
		buffer += "\n";
		buffer += header.fmt[0];
		buffer += header.fmt[1];
		buffer += header.fmt[2];
		buffer += header.fmt[3];
		buffer += "\n";
		buffer += to_string(header.Subchunk1Size);
		buffer += "\n";
		buffer += to_string(header.AudioFormat);
		buffer += "\n";
		buffer += to_string(header.NumOfChan);
		buffer += "\n";
		buffer += to_string(header.SamplesPerSec);
		buffer += "\n";
		buffer += to_string(header.bytesPerSec);
		buffer += "\n";
		buffer += to_string(header.blockAlign);
		buffer += "\n";
		buffer += to_string(header.bitsPerSample);
		buffer += "\n";
		buffer += header.Subchunk2ID[0];
		buffer += header.Subchunk2ID[1];
		buffer += header.Subchunk2ID[2];
		buffer += header.Subchunk2ID[3];
		buffer += "\n";
		buffer += to_string(header.Subchunk2Size);
		return buffer;
	}
	const char* getData() {
		return Data;
	}
	char* getDataMutable() {
		return Data;
	}

private:
	typedef struct WAV_HEADER {
		char             RIFF[4];
		unsigned long    ChunkSize;//Subchunk2Size+36Byte
		char             WAVE[4];
		char             fmt[4];
		unsigned long    Subchunk1Size;
		unsigned short   AudioFormat;//
		unsigned short   NumOfChan;//频道数
		unsigned long    SamplesPerSec;//采样率
		unsigned long    bytesPerSec;//每分钟位数
		unsigned short   blockAlign;//
		unsigned short   bitsPerSample;//每个采样多少位（Byte*8）
		char             Subchunk2ID[4];//数据
		unsigned long    Subchunk2Size;//数据长度（字节数）
		~WAV_HEADER() {}
	};//44Byte
	WAV_HEADER header;
	char* Data;
	int StartPos;
};

void ShirakanaUI::loadMods() {
	FILE* modFile = fopen("Mods\\mods.modlist", "r");
	char modName[255] = "";
	string modNameIter;
	string tmpStr = "";
	for (; fscanf(modFile, "%s", modName) != EOF;) {
		modNameIter = modName;
		size_t i = 0;
		for (; modNameIter[i] != ':'; i++) tmpStr += modNameIter[i];
		string modFileName = "Mods\\" + tmpStr + "\\" + tmpStr;
		if ((fopen((modFileName + "_decoder_iter.onnx").c_str(), "r") != nullptr)&& (fopen((modFileName + "_encoder.onnx").c_str(), "r") != nullptr)&& (fopen((modFileName + "_postnet.onnx").c_str(), "r") != nullptr)) {
			ui.ShirakanaModelSelectorOfficialComboBox->addItem(tmpStr.c_str());
			modList.push_back(modFileName);
			i++;
			tmpStr = "";
			for (; i < modNameIter.length(); i++) tmpStr += modNameIter[i];
			modNames.push_back(tmpStr);
		}
		tmpStr = "";
	}
}

ShirakanaUI::ShirakanaUI(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui.ShirakanaModelInsertOfficial, SIGNAL(clicked()), this, SLOT(OnShirakanaModelInsertOfficialClick()));
	connect(ui.ShirakanaStartButton, SIGNAL(clicked()), this, SLOT(OnShirakanaStartClick()));
	loadMods();
	if (modList.empty()) {
		ui.ShirakanaModelInsertOfficial->setEnabled(false);
	}
	//ui.ShirakanaModelSelectorOfficialComboBox->addItem("Arti");
}

ShirakanaUI::~ShirakanaUI(){
	
}

void ShirakanaUI::OnShirakanaStartClick() {
	std::string prefix = "tmpDir\\";
	if (_access(prefix.c_str(), 0) == -1)
		int returnValueMkdir = _mkdir(prefix.c_str());
	if (fopen("hifigan.onnx", "r") == nullptr) {
		QMessageBox::warning(this, "hifigan模型丢失", "hifigan模型丢失，请重新获取hifigan");
		return;
	}
	if (thisModFile.length() == 0) {
		QMessageBox::warning(this, "模型未导入", "请先导入模型！");
		return;
	}
	string inputStr = ui.ShirakanaInput->text().toStdString();
	if (inputStr.length() == 0) {
		QMessageBox::warning(this, "输入为空", "请输入要转换的字符串（仅支持假名和部分标点符号）");
		return;
	}
	if (inputStr[inputStr.length() - 1] != '.') {
		inputStr += ".";
	}
	string tempStr = "";
	string commandStr = "";
	string commandInputStr = "";
	vector <string> commandInputVec;
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString("处理输入字符串中"));
	ui.ShirakanaInput->setEnabled(false);
	//ui.DenoiseDoubleSpinBox->setEnabled(false);
	ui.ShirakanaStartButton->setEnabled(false);
	for (size_t i = 0; i < inputStr.length(); i++) {
		remove(("tmpDir\\" + to_string(i) + ".wav").c_str());
		if (((inputStr[i] < 'A') && (inputStr[i] != '.') && (inputStr[i] != ',') && (inputStr[i] != ' ')) || ((inputStr[i] > 'Z') && (inputStr[i] < 'a')) || ((inputStr[i] > 'z') && (inputStr[i] != '~'))) {
			QMessageBox::warning(this, "不支持的字符", "输入中包含不支持的字符（仅支持英文，空格，逗号和句号以及~作为延音符号）");
			ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
			ui.ShirakanaInput->setEnabled(true);
			//ui.DenoiseDoubleSpinBox->setEnabled(false);
			ui.ShirakanaStartButton->setEnabled(true);
			return;
		}
		if ((inputStr[i] == '~')) {
			if (commandInputStr != "") {
				commandInputStr += '.';
				commandInputVec.push_back(commandInputStr);
				commandInputStr = "";
			}
		}
		if ((inputStr[i] == '.')) {
			commandInputStr += '.';
			commandInputVec.push_back(commandInputStr);
			commandInputStr = "";
			break;
		}
		else {
			commandInputStr += inputStr[i];
		}
	}
	{
		QElapsedTimer t;
		t.start();
		while (t.elapsed() < 1000) {
			QCoreApplication::processEvents();
		}
	}
	for (size_t i = 0; i < commandInputVec.size(); i++) {
		ui.ShirakanaOutPutLabel->setText(QString::fromStdString("正在转换：" + commandInputVec[i]));
		commandStr = "\"Tacotron inside.exe\" \"" + thisModFile.toStdString() + "\" \"" + commandInputVec[i] + "\" \"" + std::to_string(i)+"\"";
		WinExec(commandStr.c_str(), SW_HIDE);
		FILE* isEx = nullptr;
		for (size_t j = 0; j < 600 && isEx==nullptr; j++) {
			isEx = fopen(("tmpDir\\" + to_string(i) + ".wav").c_str(),"r");
			QElapsedTimer t;
			t.start();
			while (t.elapsed() < 100) {
				QCoreApplication::processEvents();
			}
			if (j == 599) {
				QMessageBox::warning(this, "转换失败", "转换超时，可能是由于输入的单句过长且计算机配置不支持");
				ui.ProgressBar->setValue(0);
				ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
				ui.ShirakanaInput->setEnabled(true);
				//ui.DenoiseDoubleSpinBox->setEnabled(true);
				ui.ShirakanaStartButton->setEnabled(true);
				return;
			}
		}
		ui.ProgressBar->setValue(100.0 * (double)(i + 1) / (double)commandInputVec.size());
		fclose(isEx);
	}
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString("合并Wav中"));
	{
		QElapsedTimer t;
		t.start();
		while (t.elapsed() < 1000) {
			QCoreApplication::processEvents();
		}
	}
	Wav sourceData = Wav("tmpDir\\0.wav");
	if (!sourceData.isEmpty()) {
		for (size_t i = 1; i < commandInputVec.size(); i++) {
			Wav midData = Wav(("tmpDir\\" + to_string(i) + ".wav").c_str());
			if (midData.isEmpty()) {
				QMessageBox::warning(this, "输出失败", "Temp文件丢失，导致wav合并失败");
				ui.ProgressBar->setValue(0);
				ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
				ui.ShirakanaInput->setEnabled(true);
				//ui.DenoiseDoubleSpinBox->setEnabled(true);
				ui.ShirakanaStartButton->setEnabled(true);
				return;
			}
			sourceData.catWav(midData);
		}
		QString wavSaveFileName = QFileDialog::getSaveFileName(
			this,
			tr("保存文件."),
			"",
			tr("wav file(*.wav)"));
		sourceData.write(wavSaveFileName.toStdString().c_str());
		ui.ShirakanaOutPutLabel->setText(QString::fromStdString("转换完成"));
	}
	else {
		QMessageBox::warning(this, "输出失败", "Temp文件丢失，导致wav合并失败");
	}
	{
		QElapsedTimer t;
		t.start();
		while (t.elapsed() < 100) {
			QCoreApplication::processEvents();
		}
	}
	for (size_t i = 0; i < commandInputVec.size(); i++) {
		remove(("tmpDir\\" + to_string(i) + ".wav").c_str());
	}
	ui.ProgressBar->setValue(0);
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
	ui.ShirakanaInput->setEnabled(true);
	//ui.DenoiseDoubleSpinBox->setEnabled(true);
	ui.ShirakanaStartButton->setEnabled(true);
	//system(inputStr.c_str());
}

void ShirakanaUI::OnShirakanaModelInsertOfficialClick() {
	string curModPath = modList[ui.ShirakanaModelSelectorOfficialComboBox->currentIndex()];
	try {
        thisModFile = QString::fromUtf8(modList[ui.ShirakanaModelSelectorOfficialComboBox->currentIndex()]);
	}
	catch (exception e) {
		QMessageBox::warning(this, "导入失败", QString::fromUtf8(e.what()));
		return;
	}
	ui.ImageShow->setPixmap(QPixmap(QString::fromUtf8(curModPath + ".png")));
	ui.CurModel->setText(QString::fromUtf8("当前模型：" + modNames[ui.ShirakanaModelSelectorOfficialComboBox->currentIndex()]));
}
