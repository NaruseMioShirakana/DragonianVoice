#include "ShirakanaTTS-Cpp.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <QMessageBox>
#include <QFileDialog>
#include <windows.h>
#include <io.h>
#include <iostream>
#include <direct.h>
#include <QElapsedTimer>
#include <fstream>
#include <codecvt>
#include <thread>
#include <QAudioDevice>
#include <QInputDialog>
using std::string;
using std::vector;
using std::wstring;

const int HEAD_LENGTH = 3 * 1024 * 1024;//3MB_Buffer

inline std::wstring to_wide_string(const std::string& input)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(input);
}

inline std::string to_byte_string(const std::wstring& input)
{
	//std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(input);
}

LPCWSTR stringToLPCWSTR(std::string orig)
{
	size_t origsize = orig.length() + 1;
	const size_t newsize = 100;
	size_t convertedChars = 0;
	wchar_t* wcstring = (wchar_t*)malloc(sizeof(wchar_t) * (orig.length() - 1));
	mbstowcs_s(&convertedChars, wcstring, origsize, orig.c_str(), _TRUNCATE);

	return wcstring;
}

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
		if (stream != nullptr) {
			fclose(stream);
		}
	}
	Wav(const Wav& input) {
		this->Data = (char*)malloc(sizeof(char) * (input.header.Subchunk2Size + 1));
		if (this->Data == NULL) {
			throw std::exception("内存不足");
		}
		memcpy(this->header.RIFF, input.header.RIFF, 4);
		memcpy(this->header.fmt, input.header.fmt, 4);
		memcpy(this->header.WAVE, input.header.WAVE, 4);
		memcpy(this->header.Subchunk2ID, input.header.Subchunk2ID, 4);
		this->header.ChunkSize = input.header.ChunkSize;
		this->header.Subchunk1Size = input.header.Subchunk1Size;
		this->header.AudioFormat = input.header.AudioFormat;
		this->header.NumOfChan = input.header.NumOfChan;
		this->header.SamplesPerSec = input.header.SamplesPerSec;
		this->header.bytesPerSec = input.header.bytesPerSec;
		this->header.blockAlign = input.header.blockAlign;
		this->header.bitsPerSample = input.header.bitsPerSample;
		this->header.Subchunk2Size = input.header.Subchunk2Size;
		this->StartPos = input.StartPos;
		memcpy(this->Data, input.Data, input.header.Subchunk2Size);
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
			throw std::exception("RAM不足");
		}
		memcpy(this->Data, buffer, header.Subchunk2Size);
		memcpy(this->Data+ header.Subchunk2Size, input.Data, input.header.Subchunk2Size);
		free(buffer);
		this->header.ChunkSize += input.header.Subchunk2Size;
		this->header.Subchunk2Size += input.header.Subchunk2Size;
		return *this;
	}
	void write(const wchar_t* filepath) {
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
		buffer += std::to_string(header.ChunkSize);
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
		buffer += std::to_string(header.Subchunk1Size);
		buffer += "\n";
		buffer += std::to_string(header.AudioFormat);
		buffer += "\n";
		buffer += std::to_string(header.NumOfChan);
		buffer += "\n";
		buffer += std::to_string(header.SamplesPerSec);
		buffer += "\n";
		buffer += std::to_string(header.bytesPerSec);
		buffer += "\n";
		buffer += std::to_string(header.blockAlign);
		buffer += "\n";
		buffer += std::to_string(header.bitsPerSample);
		buffer += "\n";
		buffer += header.Subchunk2ID[0];
		buffer += header.Subchunk2ID[1];
		buffer += header.Subchunk2ID[2];
		buffer += header.Subchunk2ID[3];
		buffer += "\n";
		buffer += std::to_string(header.Subchunk2Size);
		return buffer;
	}
	const char* getData() {
		return Data;
	}
	char* getDataMutable() {
		return Data;
	}

private:
	struct WAV_HEADER {
		char             RIFF[4];                                       // {'R','I','F','F'}
		unsigned long    ChunkSize;//Subchunk2Size+36Byte               // 数组长度*2+36
		char             WAVE[4];                                       // {'W','A','V','E'}
		char             fmt[4];                                        // {'f','m','t',' '}
		unsigned long    Subchunk1Size;//区块大小                        // 16
		unsigned short   AudioFormat;                                   // 1
		unsigned short   NumOfChan;//频道数                             // 1
		unsigned long    SamplesPerSec;//采样率                         // 22050
		unsigned long    bytesPerSec;//每分钟比特                       // 44100
		unsigned short   blockAlign;//对其                              // 2
		unsigned short   bitsPerSample;//每个采样多少位（Byte*8）        // 16
		char             Subchunk2ID[4];//数据                          // {'D','A','T','A'}
		unsigned long    Subchunk2Size;//数据长度（字节数）              // 数组长度*2
		~WAV_HEADER() {}
	};//44Byte
	WAV_HEADER header;                                                  // int16* 强转 char* 
	char* Data; //字节数据
	int StartPos;
};

Wav curWav;

void ShirakanaUI::loadMods() {

	std::wstring prefix = L"Cleaners";
	if (_waccess(prefix.c_str(), 0) == -1)
		int returnValueMkdir = _wmkdir(prefix.c_str());

	_wfinddata_t file_info;
	wstring current_path = L"Mods\\*.mod";
	intptr_t handle = _wfindfirst(current_path.c_str(), &file_info);
	if (-1 == handle) {
		QMessageBox::warning(this, "未找到Mod", "Mod文件未找到，可能是您未安装mod。");
		return;
	}
	string modInfo;
	wstring TmpModFileName = L"";
	FILE* modfile = nullptr;
	wstring TmpModInfo = L"";
	int fileIndex = 0;
	try {
		do
		{
			if (file_info.attrib == _A_SUBDIR) {
				continue;
			}
			else {
				TmpModFileName = file_info.name;
				std::ifstream modfile((L"Mods\\" + TmpModFileName).c_str());
				if (std::getline(modfile, modInfo) && modInfo.find("Mdid:") == 0) {
					TmpModInfo = to_wide_string(modInfo);
					TmpModInfo = TmpModInfo.substr(5);
					modList.push_back(L"Mods\\" + TmpModInfo + L"\\" + TmpModInfo);
				}
				else {
					continue;
				}
				if (std::getline(modfile, modInfo) && modInfo.find("Name:") == 0) {
					TmpModInfo = to_wide_string(modInfo);
					TmpModInfo = TmpModInfo.substr(5);
					modNames.push_back(TmpModInfo);
				}
				else {
					modList.pop_back();
					continue;
				}
				if (std::getline(modfile, modInfo) && modInfo.find("Type:") == 0) {
					TmpModInfo = to_wide_string(modInfo);
					TmpModInfo = TmpModInfo.substr(5);
					modModerType.push_back(TmpModInfo);
				}
				else {
					modList.pop_back();
					modNames.pop_back();
					continue;
				}
				if (std::getline(modfile, modInfo) && modInfo.find("Symb:") == 0) {
					TmpModInfo = to_wide_string(modInfo);
					TmpModInfo = TmpModInfo.substr(5);
					modSymbol.push_back(TmpModInfo);
				}
				else {
					modList.pop_back();
					modNames.pop_back();
					modModerType.pop_back();
					continue;
				}if (std::getline(modfile, modInfo) && modInfo.find("Clen:") == 0) {
					TmpModInfo = to_wide_string(modInfo);
					TmpModInfo = TmpModInfo.substr(5);
					if (TmpModInfo == L"") {
						modCleaners.push_back(L"None");
					}
					else {
						modCleaners.push_back(L"Cleaners\\" + TmpModInfo + L".exe");
					}
				}
				else {
					modList.pop_back();
					modNames.pop_back();
					modModerType.pop_back();
					modSymbol.pop_back();
					continue;
				}
				if (modModerType[fileIndex] == L"Tacotron2") {
					if ((_wfopen((modList[fileIndex] + L"_decoder_iter.onnx").c_str(), L"r") != nullptr) && (_wfopen((modList[fileIndex] + L"_encoder.onnx").c_str(), L"r") != nullptr) && (_wfopen((modList[fileIndex] + L"_postnet.onnx").c_str(), L"r") != nullptr)) {
						ui.ShirakanaModelSelectorOfficialComboBox->addItem(QString::fromStdWString(modModerType[fileIndex] + L"::" + modNames[fileIndex]));
						fileIndex++;
						modSpeakers.push_back(vector<wstring>());
					}
					else {
						modList.pop_back();
						modNames.pop_back();
						modModerType.pop_back();
						modSymbol.pop_back();
						continue;
					}
				}
				else if (modModerType[fileIndex] == L"VITS_LJS") {
					if ((_wfopen((modList[fileIndex] + L"_LJS.pt").c_str(), L"r") != nullptr)) {
						ui.ShirakanaModelSelectorOfficialComboBox->addItem(QString::fromStdWString(modModerType[fileIndex] + L"::" + modNames[fileIndex]));
						fileIndex++;
						modSpeakers.push_back(vector<wstring>());
					}
					else {
						modList.pop_back();
						modNames.pop_back();
						modModerType.pop_back();
						modSymbol.pop_back();
						continue;
					}
				}
				else if (modModerType[fileIndex] == L"VITS_VCTK") {
					if ((_wfopen((modList[fileIndex] + L"_VCTK.pt").c_str(), L"r") != nullptr)) {
						ui.ShirakanaModelSelectorOfficialComboBox->addItem(QString::fromStdWString(modModerType[fileIndex] + L"::" + modNames[fileIndex]));
						modSpeakers.push_back(vector<wstring>());
						while (std::getline(modfile, modInfo)) {
							modSpeakers[fileIndex].push_back(to_wide_string(modInfo));
						}
						fileIndex++;
					}
					else {
						modList.pop_back();
						modNames.pop_back();
						modModerType.pop_back();
						modSymbol.pop_back();
						continue;
					}
				}
				else {
					modList.pop_back();
					modNames.pop_back();
					modModerType.pop_back();
					modSymbol.pop_back();
					continue;
				}
			}
		} while (!_wfindnext(handle, &file_info));
	}
	catch (std::exception e) {
		QMessageBox::warning(this, "错误", QString::fromStdString(e.what()));
	}
	try {
		_findclose(handle);
		if (modSymbol.size() == modModerType.size() && modModerType.size() == modNames.size() && modNames.size() == modList.size()) {
			return;
		}
		else {
			QMessageBox::warning(this, "加载失败", "因为一些未知的原因，加载失败。");
			modList.clear();
			modNames.clear();
			modModerType.clear();
			modSymbol.clear();
			ui.ShirakanaModelSelectorOfficialComboBox->clear();
		}
	}
	catch (std::exception e) {
		QMessageBox::warning(this, "警告", QString::fromStdString(e.what()));
	}
}

ShirakanaUI::ShirakanaUI(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	player = new QMediaPlayer;
	audioOutput = new QAudioOutput;
	audioOutput->setDevice(QAudioDevice());
	audioOutput->setVolume(ui.VolSlider->value());
	player->setAudioOutput(audioOutput);
	connect(ui.ShirakanaModelInsertOfficial, SIGNAL(clicked()), this, SLOT(OnShirakanaModelInsertOfficialClick()));
	connect(ui.ShirakanaStartButton, SIGNAL(clicked()), this, SLOT(OnShirakanaStartClick()));
	connect(ui.SaveButton, SIGNAL(clicked()), this, SLOT(OnWavSaveClick()));
	connect(ui.PlayerButton, SIGNAL(clicked()), this, SLOT(OnPlayerButtonClick()));
	connect(ui.VolSlider, SIGNAL(sliderMoved(int)), this, SLOT(onSliderVolumeValueChanged(int)));
	connect(player, SIGNAL(positionChanged(qint64)), this, SLOT(onSliderMusicTimeValueChanged(qint64)));
	connect(ui.PlayerSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekChange(int)));
	QObject::connect(player,SIGNAL(durationChanged(qint64)),this,SLOT(getDuration(qint64)));
	loadMods();
	try {
		ui.SaveButton->setEnabled(false);
		ui.ShirakanaInput->setEnabled(false);
		ui.ShirakanaStartButton->setEnabled(false);
		if (modList.empty()) {
			ui.ShirakanaModelInsertOfficial->setEnabled(false);
		}
	}
	catch (std::exception e) {
		QMessageBox::warning(this, "警告", QString::fromStdString(e.what()));
	}
}

ShirakanaUI::~ShirakanaUI(){
	
}

void ShirakanaUI::cleanInputs() {
	ui.ProgressBar->setValue(0);
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
	ui.ShirakanaInput->setEnabled(true);
	ui.ShirakanaStartButton->setEnabled(true);
	ui.ShirakanaModelInsertOfficial->setEnabled(true);
}

void ShirakanaUI::OnShirakanaStartClick() {

	//<InitStatus>
	std::wstring prefix = L"tmpDir";
	if (_waccess(prefix.c_str(), 0) == -1)
		int returnValueMkdir = _wmkdir(prefix.c_str());
	if (thisModFile.length() == 0) {
		QMessageBox::warning(this, "模型未导入", "请先导入模型！");
		return;
	}
	if (_wfopen(L"hifigan.onnx", L"r") == nullptr && modModerType[curModIndex] == L"Tacotron2") {
		QMessageBox::warning(this, "hifigan模型丢失", "hifigan模型丢失，请重新获取hifigan");
		return;
	}
	wstring inputStr = ui.ShirakanaInput->toMarkdown().toStdWString();
	if (inputStr.length() == 0) {
		QMessageBox::warning(this, "输入为空", "请输入要转换的字符串（仅支持假名和部分标点符号）");
		return;
	}
	std::wstring charaSets = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	isPlay = false;
	player->pause();
	ui.PlayerSlider->setValue(0);
	ui.PlayerButton->setEnabled(false);
	ui.VolSlider->setEnabled(false);
	ui.PlayerSlider->setEnabled(false);
	player->setSource(QUrl::fromLocalFile(""));
	//<!InitStatus>
	


	//<InitInput>
	inputStr = getCleanerStr(inputStr);
	while (inputStr[inputStr.length() - 1] == L'\n' || inputStr[inputStr.length() - 1] == L'\r') {
		inputStr.pop_back();
	}
	inputStr += L"\n";
	wstring tempStr = L"";
	wstring commandStr = L"";
	wstring commandInputStr = L"";
	vector <wstring> commandInputVec;
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString("处理输入字符串中"));
	ui.ShirakanaInput->setEnabled(false);
	ui.ShirakanaStartButton->setEnabled(false);
	QCoreApplication::processEvents();
	if (modCleaners[curModIndex] != L"Cleaners\\japanese_g2p.exe") {
		for (size_t i = 0; i < inputStr.length(); i++) {
			remove(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
			if (modSymbol[curModIndex].find(inputStr[i]) == std::wstring::npos && inputStr[i] != L'\n' && inputStr[i] != L'\r') {
				QMessageBox::warning(this, "不支持的字符", QString::fromStdString("字符位置：[" + std::to_string(i + 1) + "]"));
				cleanInputs();
				return;
			}
			if ((inputStr[i] == L'\n') || (inputStr[i] == L'\r')) {
				if (commandInputStr != L"") {
					if (charaSets.find(commandInputStr[commandInputStr.length() - 1]) != wstring::npos) {
						commandInputStr += L'.';
					}
					if (commandInputStr.length() > 101) {
						QMessageBox::warning(this, "转换失败", QString::fromStdString(("单句超出最大长度（100Byte）\n位于Index[" + std::to_string(commandInputVec.size()) + "]位置")));
						cleanInputs();
						return;
					}
					commandInputVec.push_back(commandInputStr);
					commandInputStr = L"";
				}
			}
			else {
				commandInputStr += inputStr[i];
			}
		}
	}
	else if (modCleaners[curModIndex] != L"None") {
		bool bRet = false;
		wstring inputStrTmp1 = QInputDialog::getMultiLineText(this, "自定义音素","输入：",QString::fromStdWString(inputStr), &bRet).toStdWString();
		if (bRet == true) {
			inputStr = inputStrTmp1;
		}
		if (inputStr[inputStr.size() - 1] != L'\n' && inputStr[inputStr.size() - 1] != L'\r') {
			inputStr += L'\n';
		}
		for (size_t i = 0; i < inputStr.length(); i++) {
			remove(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
			if (modSymbol[curModIndex].find(inputStr[i]) == std::wstring::npos && inputStr[i] != L'\n' && inputStr[i] != L'\r') {
				QMessageBox::warning(this, "不支持的字符", QString::fromStdString("字符位置：[" + std::to_string(i + 1) + "]"));
				cleanInputs();
				return;
			}
			if ((inputStr[i] == L'\n') || (inputStr[i] == L'\r')) {
				if (commandInputStr != L"") {
					if (charaSets.find(commandInputStr[commandInputStr.length() - 1]) != wstring::npos) {
						commandInputStr += L'.';
					}
					if (commandInputStr.length() > 101) {
						QMessageBox::warning(this, "转换失败", QString::fromStdString(("单句超出最大长度（100Byte）\n位于Index[" + std::to_string(commandInputVec.size()) + "]位置")));
						cleanInputs();
						return;
					}
					commandInputVec.push_back(commandInputStr);
					commandInputStr = L"";
				}
			}
			else {
				commandInputStr += inputStr[i];
			}
		}
	}
	//<InitInput>

	ui.ShirakanaModelInsertOfficial->setEnabled(false);

	//<VITS_LJS>
	if (modModerType[curModIndex] == L"VITS_LJS") {
		for (size_t i = 0; i < commandInputVec.size(); i++) {
			if (commandInputVec[i].length() > 10) {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdWString(L"正在转换：" + commandInputVec[i].substr(0, 10) + L"..."));
			}
			else {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdWString(L"正在转换：" + commandInputVec[i]));
			}
			commandStr = L"\"VITS-LibTorch.exe\" \""
				+ thisModFile.toStdWString()
				+ L"\" \""
				+ commandInputVec[i]
				+ L"\" \""
				+ std::to_wstring(i)
				+ L"\" \""
				+ modSymbol[curModIndex]
				+ L"\" \"LJS\"";
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(si));
				ZeroMemory(&pi, sizeof(pi));
				CreateProcess(NULL,   // No module name (use command line)
					const_cast<LPWSTR>(commandStr.c_str()),        // Command line
					NULL,           // Process handle not inheritable
					NULL,           // Thread handle not inheritable
					FALSE,          // Set handle inheritance to FALSE
					CREATE_NO_WINDOW,              // No creation flags
					NULL,           // Use parent's environment block
					NULL,           // Use parent's starting directory 
					&si,            // Pointer to STARTUPINFO structure
					&pi);
				//WinExec(commandStr.c_str(), SW_HIDE);
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			{
				QElapsedTimer t;
				t.start();
				while (t.elapsed() < 2) {
					QCoreApplication::processEvents();
				}
			}
			ui.ProgressBar->setValue(100.0 * (double)(i + 1) / (double)commandInputVec.size());
		}
	}
	//<!VITS_LJS>



	//<VITS_VCTK>
	else if (modModerType[curModIndex] == L"VITS_VCTK") {
		int Speaker = ui.SpeakerSelector->currentIndex();
		for (size_t i = 0; i < commandInputVec.size(); i++) {
			if (commandInputVec[i].length() > 10) {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdWString(L"正在转换：" + commandInputVec[i].substr(0, 10) + L"..."));
			}
			else {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdWString(L"正在转换：" + commandInputVec[i]));
			}
			commandStr = L"\"VITS-LibTorch.exe\" \""
				+ thisModFile.toStdWString()
				+ L"\" \""
				+ commandInputVec[i]
				+ L"\" \""
				+ std::to_wstring(i)
				+ L"\" \""
				+ modSymbol[curModIndex]
				+ L"\" \"VCTK\" \""
				+ std::to_wstring(Speaker)
				+ L"\"";
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(si));
				ZeroMemory(&pi, sizeof(pi));
				CreateProcess(NULL,   // No module name (use command line)
					const_cast<LPWSTR>(commandStr.c_str()),        // Command line
					NULL,           // Process handle not inheritable
					NULL,           // Thread handle not inheritable
					FALSE,          // Set handle inheritance to FALSE
					CREATE_NO_WINDOW,              // No creation flags
					NULL,           // Use parent's environment block
					NULL,           // Use parent's starting directory 
					&si,            // Pointer to STARTUPINFO structure
					&pi);
				//WinExec(commandStr.c_str(), SW_HIDE);
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			{
				QElapsedTimer t;
				t.start();
				while (t.elapsed() < 2) {
					QCoreApplication::processEvents();
				}
			}
			ui.ProgressBar->setValue(100.0 * (double)(i + 1) / (double)commandInputVec.size());
		}
	}
	//<!VITS_VCTK>

	

	//<Tacotron2>
	else if (modModerType[curModIndex] == L"Tacotron2") {
		for (size_t i = 0; i < commandInputVec.size(); i++) {
			if (commandInputVec[i].length() > 10) {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdWString(L"正在转换：" + commandInputVec[i].substr(0, 10) + L"..."));
			}
			else {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdWString(L"正在转换：" + commandInputVec[i]));
			}
			commandStr = L"\"Tacotron inside.exe\" \""
				+ thisModFile.toStdWString()
				+ L"\" \""
				+ commandInputVec[i]
				+ L"\" \""
				+ std::to_wstring(i)
				+ L"\" \""
				+ modSymbol[curModIndex]
				+ L"\"";
			FILE* isOut = nullptr;
			for (size_t k = 0;; k++) {
				remove("decoder");
				{
					STARTUPINFO si;
					PROCESS_INFORMATION pi;
					ZeroMemory(&si, sizeof(si));
					ZeroMemory(&pi, sizeof(pi));
					CreateProcess(NULL,   // No module name (use command line)
						const_cast<LPWSTR>(commandStr.c_str()),        // Command line
						NULL,           // Process handle not inheritable
						NULL,           // Thread handle not inheritable
						FALSE,          // Set handle inheritance to FALSE
						CREATE_NO_WINDOW,              // No creation flags
						NULL,           // Use parent's environment block
						NULL,           // Use parent's starting directory 
						&si,            // Pointer to STARTUPINFO structure
						&pi);
					//WinExec(commandStr.c_str(), SW_HIDE);
					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
				{
					QElapsedTimer t;
					t.start();
					while (t.elapsed() < 2) {
						QCoreApplication::processEvents();
					}
				}
				isOut = fopen("decoder", "r");
				if (isOut == nullptr) {
					break;
				}
				else {
					fclose(isOut);
					remove("decoder");
					if (k == 4) {
						QMessageBox::warning(this, "转换失败", QString::fromStdString("解码步数超限\n试图减少第" + std::to_string(i + 1) + "句的长度"));
						remove("decoder");
						cleanInputs();
						return;
					}
					else {
						QMessageBox::warning(this, "解码步数超限", QString::fromStdString("解码步数超限\n重试第" + std::to_string(k + 1) + "次"));
					}
				}
			}
			ui.ProgressBar->setValue(100.0 * (double)(i + 1) / (double)commandInputVec.size());
			if (isOut != nullptr) {
				fclose(isOut);
			}
		}
	}
	//<!Tacotron2>



	//<Wav合并>
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString("合并Wav中"));
	try {
		Wav sourceData = Wav("tmpDir\\0.wav");
		if (!sourceData.isEmpty()) {
			for (size_t i = 1; i < commandInputVec.size(); i++) {
				Wav midData = Wav(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
				if (midData.isEmpty()) {
					QMessageBox::warning(this, "合并失败", "Temp文件丢失，导致wav合并失败");
					cleanInputs();
					return;
				}
				sourceData.catWav(midData);
			}
			curWav = Wav(sourceData);
		}
		else {
			QMessageBox::warning(this, "合并失败", "Temp文件丢失，导致wav合并失败");
			cleanInputs();
			return;
		}
	}
	catch (std::exception e) {
		QMessageBox::warning(this, "输出失败", QString::fromStdString(e.what()));
	}
	{
		QElapsedTimer t;
		t.start();
		while (t.elapsed() < 100) {
			QCoreApplication::processEvents();
		}
	}
	//<!Wav合并>



	//<恢复初始状态>
	for (size_t i = 0; i < commandInputVec.size(); i++) {
		remove(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
	}
	ui.ProgressBar->setValue(0);
	ui.SaveButton->setEnabled(true);
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString("音频已写入内存"));
	ui.ShirakanaInput->setEnabled(true);
	ui.ShirakanaStartButton->setEnabled(true);
	ui.ShirakanaModelInsertOfficial->setEnabled(true);
	//<!恢复初始状态>



	//<输出临时文件>
	remove("tmpDir\\Temp.wav");
	curWav.write(L"tmpDir\\Temp.wav");
	player->setSource(QUrl::fromLocalFile("tmpDir\\Temp.wav"));
	ui.PlayerButton->setEnabled(true);
	ui.VolSlider->setEnabled(true);
	ui.PlayerSlider->setEnabled(true);
	//<!输出临时文件>
}

void ShirakanaUI::OnShirakanaModelInsertOfficialClick() {
	curModIndex = ui.ShirakanaModelSelectorOfficialComboBox->currentIndex();
	wstring curModPath = modList[curModIndex];
	try {
        thisModFile = QString::fromStdWString(modList[curModIndex]);
	}
	catch (std::exception e) {
		QMessageBox::warning(this, "导入失败", QString::fromUtf8(e.what()));
		return;
	}
	ui.ImageShow->setPixmap(QPixmap(QString::fromStdWString(curModPath + L".png")));
	ui.CurModel->setText(QString::fromStdWString(L"当前模型：" + modNames[curModIndex]));
	ui.ShirakanaInput->setEnabled(true);
	ui.ShirakanaStartButton->setEnabled(true);
	ui.SpeakerSelector->clear();
	if (modModerType[curModIndex] == L"VITS_VCTK") {
		ui.SpeakerSelector->setEnabled(true);
		for (auto speaker : modSpeakers[curModIndex]) {
			ui.SpeakerSelector->addItem(QString::fromStdWString(speaker));
		}
	}
	else {
		ui.SpeakerSelector->setEnabled(false);
	}
}

void ShirakanaUI::OnWavSaveClick() {
	QString wavSaveFileName = QFileDialog::getSaveFileName(
		this,
		tr("保存文件."),
		"",
		tr("wav file(*.wav)"));
	remove(wavSaveFileName.toStdString().c_str());
	curWav.write(wavSaveFileName.toStdWString().c_str());
}

void ShirakanaUI::onSliderVolumeValueChanged(int value) {
	audioOutput->setVolume(value);
}

void ShirakanaUI::onSliderMusicTimeValueChanged(qint64 value) {
	ui.PlayerSlider->setValue(value);
	if (value == player->duration()) {
		ui.ShirakanaStartButton->setEnabled(true);
		isPlay = false;
		ui.PlayerSlider->setValue(0);
	}
}

void ShirakanaUI::seekChange(int position)
{
	player->setPosition(position);
}

void ShirakanaUI::OnPlayerButtonClick() {
	if (!isPlay) {
		isPlay = true;
		ui.ShirakanaStartButton->setEnabled(false);
		player->play();
	}
	else {
		isPlay = false;
		ui.ShirakanaStartButton->setEnabled(true);
		player->pause();
	}
}

void ShirakanaUI::getDuration(qint64 time) {
	ui.PlayerSlider->setMaximum(player->duration());
}

wstring ShirakanaUI::getCleanerStr(wstring input) {
	wstring output =L"";
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0) || !SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0))
	{
		QMessageBox::warning(this, "错误", "创建管道失败");
		return input;
	}
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.dwFlags |= STARTF_USESTDHANDLES;
	wstring commandStr = L"";
	if (modCleaners[curModIndex] == L"Cleaners\\japanese_g2p.exe") {
		commandStr = L"\""
			+ modCleaners[curModIndex]
			+ L"\" -rsa \""
			+ input
			+ L"\"";
	}
	else {
		commandStr = L"\""
			+ modCleaners[curModIndex]
			+ L"\" \""
			+ input
			+ L"\"";
	}
	
	auto ProcessSta = CreateProcess(NULL,   
		const_cast<LPWSTR>(commandStr.c_str()),        
		NULL,           
		NULL,           
		TRUE,          
		CREATE_NO_WINDOW,              
		NULL,           
		NULL,           
		&si,            
		&pi);
	if (!ProcessSta) {
		CloseHandle(hWrite);
		CloseHandle(hRead);
		return input;
	}
	CloseHandle(hWrite);
	char ReadBuff[1025] = "";
	DWORD bytesRead;
	while (ReadFile(hRead, ReadBuff, 1024, &bytesRead, NULL))
	{
		ReadBuff[bytesRead] = '\0';
		output += to_wide_string(ReadBuff);
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(hRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return output;
}