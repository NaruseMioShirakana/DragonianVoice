#include "ShirakanaTTS-Cpp.h"
#include <string>
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
using std::string;
using std::vector;

const int HEAD_LENGTH = 3 * 1024 * 1024;//3MB_Buffer

inline std::wstring to_wide_string(const std::string& input)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(input);
}
// convert wstring to string 
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
	_finddata_t file_info;
	string current_path = "Mods\\*.mod";
	intptr_t handle = _findfirst(current_path.c_str(), &file_info);
	if (-1 == handle) {
		QMessageBox::warning(this, "未找到Mod", "Mod文件未找到，可能是您未安装mod。");
		return;
	}
	char modInfo[255] = "";
	string TmpModFileName = "";
	FILE* modfile = nullptr;
	string TmpModInfo = "";
	int fileIndex = 0;
	do
	{
		if (file_info.attrib == _A_SUBDIR) {
			continue;
		}
		else {
			TmpModFileName = file_info.name;
			modfile = fopen(("Mods\\" + TmpModFileName).c_str(), "r");
			if (fscanf(modfile, "%s", modInfo) != EOF) {
				TmpModInfo = modInfo;
				TmpModInfo = TmpModInfo.substr(5);
				modList.push_back("Mods\\" + TmpModInfo + "\\" + TmpModInfo);
			}
			else {
				continue;
			}
			if (fscanf(modfile, "%s", modInfo) != EOF) {
				TmpModInfo = modInfo;
				TmpModInfo = TmpModInfo.substr(5);
				modNames.push_back(TmpModInfo);
			}
			else {
				modList.pop_back();
				continue;
			}
			if (fscanf(modfile, "%s", modInfo) != EOF) {
				TmpModInfo = modInfo;
				TmpModInfo = TmpModInfo.substr(5);
				modModerType.push_back(TmpModInfo);
			}
			else {
				modList.pop_back();
				modNames.pop_back();
				continue;
			}
			if (fscanf(modfile, "%s", modInfo) != EOF) {
				TmpModInfo = modInfo;
				TmpModInfo = TmpModInfo.substr(5);
				modSymbol.push_back(TmpModInfo);
			}
			else {
				modList.pop_back();
				modNames.pop_back();
				modModerType.pop_back();
				continue;
			}
			if (fscanf(modfile, "%s", modInfo) != EOF) {
				TmpModInfo = modInfo;
				modSymbol[fileIndex] += " "+TmpModInfo;
			}
			if (modModerType[fileIndex] == "Tacotron2") {
				if ((fopen((modList[fileIndex] + "_decoder_iter.onnx").c_str(), "r") != nullptr) && (fopen((modList[fileIndex] + "_encoder.onnx").c_str(), "r") != nullptr) && (fopen((modList[fileIndex] + "_postnet.onnx").c_str(), "r") != nullptr)) {
					ui.ShirakanaModelSelectorOfficialComboBox->addItem((modModerType[fileIndex]+"::" + modNames[fileIndex]).c_str());
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
			else if (modModerType[fileIndex] == "VITS_LJS") {
				if ((fopen((modList[fileIndex] + "_ljs.onnx").c_str(), "r") != nullptr)) {
					ui.ShirakanaModelSelectorOfficialComboBox->addItem((modModerType[fileIndex] + "::" + modNames[fileIndex]).c_str());
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
			else if (modModerType[fileIndex] == "VITS_VCTK") {
				if ((fopen((modList[fileIndex] + "_vctk.onnx").c_str(), "r") != nullptr)) {
					ui.ShirakanaModelSelectorOfficialComboBox->addItem((modModerType[fileIndex] + "::" + modNames[fileIndex]).c_str());
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
	} while (!_findnext(handle, &file_info));
	_findclose(handle);
	if (modSymbol.size() == modModerType.size() && modModerType.size() == modNames.size() && modNames.size() == modList.size()) {
		QMessageBox::information(this, "加载完毕", QString::fromStdString("加载了：" + std::to_string(modSymbol.size()) + "个Model"));
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
}

ShirakanaUI::~ShirakanaUI(){
	
}

void ShirakanaUI::cleanInputs() {
	ui.ProgressBar->setValue(0);
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
	ui.ShirakanaInput->setEnabled(true);
	ui.ShirakanaStartButton->setEnabled(true);
}

void ShirakanaUI::OnShirakanaStartClick() {


	//<InitStatus>
	std::string prefix = "tmpDir\\";
	if (_access(prefix.c_str(), 0) == -1)
		int returnValueMkdir = _mkdir(prefix.c_str());
	if (thisModFile.length() == 0) {
		QMessageBox::warning(this, "模型未导入", "请先导入模型！");
		return;
	}
	if (fopen("hifigan.onnx", "r") == nullptr && modModerType[curModIndex] == "Tacotron2") {
		QMessageBox::warning(this, "hifigan模型丢失", "hifigan模型丢失，请重新获取hifigan");
		return;
	}
	string inputStr = ui.ShirakanaInput->text().toStdString();
	if (inputStr.length() == 0) {
		QMessageBox::warning(this, "输入为空", "请输入要转换的字符串（仅支持假名和部分标点符号）");
		return;
	}
	//<!InitStatus>



	//<InitInput>
	if (inputStr[inputStr.length() - 1] == '~') {
		inputStr[inputStr.length() - 1] = '.';
	}
	else if (inputStr[inputStr.length() - 1] != '.') {
		inputStr += ".";
	}
	string tempStr = "";
	string commandStr = "";
	string commandInputStr = "";
	vector <string> commandInputVec;
	//<InitInput>



	//<VITS_LJS>
	if (modModerType[curModIndex] == "VITS_LJS") {
		QMessageBox::information(this, "TODO", "TODO");
		cleanInputs();
		return;
	}
	//<!VITS_LJS>



	//<VITS_VCTK>
	else if (modModerType[curModIndex] == "VITS_VCTK") {
		QMessageBox::information(this, "TODO", "TODO");
		cleanInputs();
		return;
	}
	//<!VITS_VCTK>



	//<Tacotron2>
	else if (modModerType[curModIndex] == "Tacotron2") {
		ui.ShirakanaOutPutLabel->setText(QString::fromStdString("处理输入字符串中"));
		ui.ShirakanaInput->setEnabled(false);
		ui.ShirakanaStartButton->setEnabled(false);
		for (size_t i = 0; i < inputStr.length(); i++) {
			remove(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
			if (modSymbol[curModIndex].find(inputStr[i]) == std::string::npos&& inputStr[i]!='~') {
				QMessageBox::warning(this, "不支持的字符", QString::fromStdString("字符位置：["+std::to_string(i+1)+"]"));
				cleanInputs();
				return;
			}
			if ((inputStr[i] == '~')) {
				if (commandInputStr != "") {
					commandInputStr += '.';
					if (commandInputStr.length() > 151) {
						QMessageBox::warning(this, "转换失败", "单句超出最大长度（150Byte）");
						cleanInputs();
						return;
					}
					commandInputVec.push_back(commandInputStr);
					commandInputStr = "";
				}
			}
			else if ((inputStr[i] == '.')) {
				commandInputStr += '.';
				if (commandInputStr.length() > 151) {
					QMessageBox::warning(this, "转换失败", "单句超出最大长度（150Byte）");
					cleanInputs();
					return;
				}
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
			if (commandInputVec[i].length() > 10) {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdString("正在转换：" + commandInputVec[i].substr(0, 10) + "..."));
			}
			else {
				ui.ShirakanaOutPutLabel->setText(QString::fromStdString("正在转换：" + commandInputVec[i]));
			}
			commandStr = "\"Tacotron inside.exe\" \"" 
				+ thisModFile.toStdString() 
				+ "\" \"" 
				+ commandInputVec[i] 
				+ "\" \"" 
				+ std::to_string(i) 
				+ "\" \""
				+ modSymbol[curModIndex]
				+ "\"";
			FILE* isEx = nullptr;
			FILE* isOut = nullptr;
			for (size_t k = 0;; k++) {
				remove("decoder");
				{
					STARTUPINFO si;
					PROCESS_INFORMATION pi;
					ZeroMemory(&si, sizeof(si));
					ZeroMemory(&pi, sizeof(pi));
					CreateProcess(NULL,   // No module name (use command line)
						const_cast<LPWSTR>(to_wide_string(commandStr).c_str()),        // Command line
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
					while (t.elapsed() < 10) {
						QCoreApplication::processEvents();
					}
				}
				isOut = fopen("decoder", "r");
				if (isOut == nullptr) {
					break;
				}
				else {
					QMessageBox::warning(this, "解码步数超限", QString::fromStdString("解码步数超限\n重试第" + std::to_string(k+1) + "次"));
					if (k == 4) {
						QMessageBox::warning(this, "转换失败", QString::fromStdString("解码步数超限\n试图减少第" + std::to_string(i) + "句的长度"));
						remove("decoder");
						cleanInputs();
						return;
					}
				}
			}
			ui.ProgressBar->setValue(100.0 * (double)(i + 1) / (double)commandInputVec.size());
			if (isEx != nullptr) {
				fclose(isEx);
			}
			if (isOut != nullptr) {
				fclose(isOut);
			}
		}
	}
	//<!Tacotron2>



	//<Wav合并及输出>
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString("合并Wav中"));
	{
		QElapsedTimer t;
		t.start();
		while (t.elapsed() < 1000) {
			QCoreApplication::processEvents();
		}
	}
	try {
		Wav sourceData = Wav("tmpDir\\0.wav");
		if (!sourceData.isEmpty()) {
			for (size_t i = 1; i < commandInputVec.size(); i++) {
				Wav midData = Wav(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
				if (midData.isEmpty()) {
					QMessageBox::warning(this, "输出失败", "Temp文件丢失，导致wav合并失败");
					cleanInputs();
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
	//<!Wav合并及输出>



	//<恢复初始状态>
	for (size_t i = 0; i < commandInputVec.size(); i++) {
		remove(("tmpDir\\" + std::to_string(i) + ".wav").c_str());
	}
	ui.ProgressBar->setValue(0);
	ui.ShirakanaOutPutLabel->setText(QString::fromStdString(""));
	ui.ShirakanaInput->setEnabled(true);
	ui.ShirakanaStartButton->setEnabled(true);
	//<!恢复初始状态>
}

void ShirakanaUI::OnShirakanaModelInsertOfficialClick() {
	curModIndex = ui.ShirakanaModelSelectorOfficialComboBox->currentIndex();
	string curModPath = modList[curModIndex];
	try {
        thisModFile = QString::fromUtf8(modList[curModIndex]);
	}
	catch (std::exception e) {
		QMessageBox::warning(this, "导入失败", QString::fromUtf8(e.what()));
		return;
	}
	ui.ImageShow->setPixmap(QPixmap(QString::fromUtf8(curModPath + ".png")));
	ui.CurModel->setText(QString::fromUtf8("当前模型：" + modNames[curModIndex]));
}
