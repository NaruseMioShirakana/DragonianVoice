#include "ttsmainwindow.h"
#include <QMessageBox>
#include "ui_ttsmainwindow.h"
#include <QStringListModel>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <thread>

TTSMainWindow::TTSMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TTSMainWindow),
    exit_callback(_callback)
{
    ui->setupUi(this);
    //setWindowFlags();
    initSetup();
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    connect(media_player, SIGNAL(durationChanged(qint64)), this, SLOT(getDurationChanged(qint64)));
    connect(media_player, SIGNAL(positionChanged(qint64)), this, SLOT(playbackPosChanged(qint64)));
    connect(&_InferMsgSender, SIGNAL(onProcessBarChanging(size_t, size_t)), this, SLOT(InferenceProcessChanged(size_t, size_t)));
    connect(&_InferMsgSender, SIGNAL(onAnyStatChanging(bool)), this, SLOT(InferenceStatChanged(bool)));
    connect(&_InferMsgSender, SIGNAL(astrParamedEvent(std::string)), this, SLOT(ErrorMsgBoxEvent(std::string)));
    connect(&_InferMsgSender, SIGNAL(strParamedEvent(std::wstring)), this, SLOT(InsertFilePaths(std::wstring)));
    connect(&_InferMsgSender, SIGNAL(mPaintEvent()), this, SLOT(PaintSpec()));
}

TTSMainWindow::~TTSMainWindow()
{
    delete ui;
    delete wave_widget_;
    delete wave_widget_layout;
    delete media_player;
    delete media_audio_output;
}

void TTSMainWindow::SetInferenceEnabled(bool condition) const
{
    ui->BatchedTTSButton->setEnabled(condition);
    ui->InferenceButton->setEnabled(condition);
    ui->TTSInferOnceButton->setEnabled(condition);
    ui->TTSInferCur->setEnabled(condition);
}

void TTSMainWindow::closeEvent(QCloseEvent* e)
{
    
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("TTSMainMenuQuitTitle"));
    msgBox.setText(tr("TTSMainMenuQuitText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("TTSMainMenuQuitYes"));
    msgBox.button(QMessageBox::No)->setText(tr("TTSMainMenuQuitNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
        e->accept();
    else
        e->ignore();
}

void TTSMainWindow::unloadModel()
{
    SetInferenceEnabled(false);
    SetCharaMixEnabled(false);

    ui->TextCleaner->setEnabled(false);
    ui->TTSInferenceCleanerButton->setEnabled(false);
    delete _model;
    _model = nullptr;
    charaMixData.clear();
    ui->CharacterComboBox->clear();
    ui->CharacterComboBox->setEnabled(false);
    n_speakers = 0;
}

void TTSMainWindow::SetCharaMixEnabled(bool condition) const
{
    ui->CharacterCheckBox->setCheckState(Qt::Unchecked);
    ui->CharacterCheckBox->setEnabled(condition);
    ui->CharacterMixProportion->setEnabled(condition);
}

void TTSMainWindow::SetParamsEnabled(bool condition) const
{
    ui->GateThSlider->setEnabled(condition);
    ui->LengthScaleSlider->setEnabled(condition);
    ui->MaxDecodeStepSlider->setEnabled(condition);
    ui->NoiseScaleSlider->setEnabled(condition);
    ui->NoiseScaleWSlider->setEnabled(condition);
    ui->GateThSpinBox->setEnabled(condition);
    ui->LengthScaleSpinBox->setEnabled(condition);
    ui->MaxDecodeStepSpinBox->setEnabled(condition);
    ui->NoiseScaleSpinBox->setEnabled(condition);
    ui->NoiseScaleWSpinBox->setEnabled(condition);
    ui->EmotionEdit->setEnabled(condition);
}

void TTSMainWindow::initSetup()
{
    setAcceptDrops(true);
    ui->TTSParamSeedEdit->setValidator(new QIntValidator(ui->TTSParamSeedEdit));
    initControls();
    SetParamsEnabled(true);
    SetCharaMixEnabled(false);
    SetInferenceEnabled(false);
    ui->CharacterComboBox->setEnabled(false);
    initPlayer();
    reloadModels();
}

void TTSMainWindow::initControls()
{
    wave_widget_layout = new QGridLayout(ui->AudioWaveWidget);
    wave_widget_layout->setObjectName("wave_widget_layout");
    wave_widget_ = new waveWidget(ui->AudioWaveWidget);
    wave_widget_->setObjectName("wave_widget_");
    wave_widget_layout->addWidget(wave_widget_);
    ui->AudioWaveWidget->setLayout(wave_widget_layout);
}

void TTSMainWindow::modelsClear()
{
    unloadModel();
    ui->ModelComboBox->clear();
    _models.clear();
}

void TTSMainWindow::SetModelSelectEnabled(bool condition) const
{
    ui->LoadModelButton->setEnabled(condition);
    ui->ModelListRefreshButton->setEnabled(condition);
}

void TTSMainWindow::reloadModels()
{
    modelsClear();
    ui->ModelComboBox->setEnabled(false);
    std::wstring prefix = L"Cleaners";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create Cleaners Dir");
    prefix = L"emotion";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create emotion Dir");
    prefix = L"Project";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create Project Dir");
    prefix = L"Models";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create Models Dir");
    prefix = L"outputs";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create temp Dir");
    prefix = L"hifigan";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create hifigan Dir");
    prefix = L"hubert";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create hubert Dir");
    prefix = L"dict";
    if (_waccess(prefix.c_str(), 0) == -1)
        if (_wmkdir(prefix.c_str()))
            logger.log(L"[Info] Create dict Dir");
    _wfinddata_t file_info;
    std::wstring current_path = GetCurrentFolder() + L"\\Models";
    intptr_t handle = _wfindfirst((current_path + L"\\*.json").c_str(), &file_info);
    if (-1 != handle) {
        ui->ModelComboBox->addItem("None");
        do
        {
            std::string modInfo, modInfoAll;
            std::ifstream modfile((current_path + L"\\" + file_info.name).c_str());
            while (std::getline(modfile, modInfo))
                modInfoAll += modInfo;
            modfile.close();
            rapidjson::Document modConfigJson;
            modConfigJson.Parse(modInfoAll.c_str());
            if (modConfigJson.HasParseError() ||
                !modConfigJson.HasMember("Folder") ||
                !modConfigJson.HasMember("Name") ||
                !modConfigJson.HasMember("Type") ||
                !modConfigJson.HasMember("Rate") ||
                !modConfigJson["Folder"].IsString() ||
                !modConfigJson["Name"].IsString() ||
                !modConfigJson["Type"].IsString() ||
                !(modConfigJson["Rate"].IsInt() || modConfigJson["Rate"].IsInt64()) ||
                !modConfigJson["Folder"].GetStringLength() ||
                !modConfigJson["Name"].GetStringLength() ||
                !modConfigJson["Type"].GetStringLength() ||
                modConfigJson["Rate"].IsNull()
                )
                continue;

            std::string Name = modConfigJson["Name"].GetString();
            std::string Type = modConfigJson["Type"].GetString();
            if (Type == "VITS_LJS" || Type == "VITS_VCTK")
            {
                Type = "Vits";
                modConfigJson["Type"].SetString("Vits");
            }

            if (Type != "Tacotron" && Type != "Tacotron2" && Type != "Vits" && Type != "Pits")
                continue;
            auto _tmpText = Type + ":" + Name;
            ui->ModelComboBox->addItem(_tmpText.c_str());
            _models.emplace_back(std::move(modConfigJson));
        } while (!_wfindnext(handle, &file_info));
        if (!_models.empty())
            ui->ModelComboBox->setEnabled(true);
    }

    current_path = GetCurrentFolder() + L"\\Cleaners";
	handle = _wfindfirst((current_path + L"\\*.dll").c_str(), &file_info);
    if (-1 != handle) {
        ui->G2pComboBox->addItem("None");
        do
        {
        	auto path = current_path + L"\\" + file_info.name;
            ui->G2pComboBox->addItem(to_byte_string(file_info.name).c_str());
            CleanerPaths.emplace_back(path);
        } while (!_wfindnext(handle, &file_info));
        if (!CleanerPaths.empty())
            ui->G2pComboBox->setEnabled(true);
    }

    current_path = GetCurrentFolder() + L"\\dict";
    handle = _wfindfirst((current_path + L"\\*.json").c_str(), &file_info);
    if (-1 != handle) {
        ui->DictComboBox->addItem("None");
        do
        {
            auto path = current_path + L"\\" + file_info.name;
            ui->DictComboBox->addItem(to_byte_string(file_info.name).c_str());
            DictPaths.emplace_back(path);
        } while (!_wfindnext(handle, &file_info));
        if (!DictPaths.empty())
            ui->DictComboBox->setEnabled(true);
    }
}

void TTSMainWindow::loadModel(size_t idx)
{
    if (idx == cur_model_index)
        return;
    unloadModel();
	if(idx == 0)
	{
        cur_model_index = 0;
		return;
	}
    const auto index_model = idx - 1;
    const auto& Config = _models[index_model];
    try
    {
        const std::string _type_model = Config["Type"].GetString();
        if (_type_model == "Tacotron2")
        {
            _model = dynamic_cast<InferClass::TTS*>(new InferClass::Tacotron2(Config, BarCallback, TTSParamCallback, [&](std::vector<float>& _inp) {}, __MOESS_DEVICE));
        }
        else if (_type_model == "Vits")
        {
            _model = dynamic_cast<InferClass::TTS*>(new InferClass::Vits(Config, BarCallback, TTSParamCallback, [&](std::vector<float>& _inp) {}, __MOESS_DEVICE));
        }
        else if (_type_model == "Pits")
        {
            _model = dynamic_cast<InferClass::TTS*>(new InferClass::Pits(Config, BarCallback, TTSParamCallback, [&](std::vector<float>& _inp) {}, __MOESS_DEVICE));
        }
        else
        {
            QMessageBox::warning(this, "ERROR", "Unsupported model", QMessageBox::Ok);
            ui->ModelComboBox->setCurrentIndex(0);
            cur_model_index = 0;
            return;
        }
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
        ui->ModelComboBox->setCurrentIndex(0);
        cur_model_index = 0;
        return;
    }
    samplingRate = _model->GetSamplingRate();
    if(Config.HasMember("Characters"))
    {
	    if (Config["Characters"].IsArray() && !Config["Characters"].Empty())
	    {
	    	ui->CharacterComboBox->setEnabled(true);
	    	for (auto& it : Config["Characters"].GetArray())
	    		ui->CharacterComboBox->addItem(it.GetString());
	    	n_speakers = Config["Characters"].Size();
	    	if (n_speakers > 1)
	    		SetCharaMixEnabled(true);
	    }
    }
    cur_model_index = idx;
    SetInferenceEnabled(true);
    if(n_speakers > 1)
    {
	    charaMixData = std::vector(n_speakers, 0.f);
        charaMixData[0] = 1.f;
        ui->CharacterMixProportion->setValue(1.0);
    }
}

void TTSMainWindow::setParamsControlValue(const MoeVSProject::TTSParams& _params)
{
    ui->NoiseScaleSpinBox->setValue(_params.noise);
    ui->NoiseScaleWSpinBox->setValue(_params.noise_w);
    ui->LengthScaleSpinBox->setValue(_params.length);
    ui->GateThSpinBox->setValue(_params.gate);
    ui->MaxDecodeStepSpinBox->setValue(int(_params.decode_step));
    ui->EmotionEdit->setText(to_byte_string(_params.emotion).c_str());
    for (size_t i = 0; i < charaMixData.size() && i < _params.chara_mix.size(); ++i)
        charaMixData[i] = _params.chara_mix[i];
    ui->SrcTextEdit->setText(to_byte_string(_params.phs).c_str());
    std::string duration;
    for (const auto& i : _params.durations)
        duration += std::to_string(i) + ' ';
    ui->DurationEdit->setText(duration.c_str());
    std::string tone;
    for (const auto& i : _params.tones)
        tone += std::to_string(i) + ' ';
    ui->ToneEdit->setText(tone.c_str());
    ui->TTSParamSeedEdit->setText(std::to_string(_params.seed).c_str());
}

MoeVSProject::TTSParams TTSMainWindow::dumpParamsFromControl() const
{
    MoeVSProject::TTSParams _params;
    _params.noise = ui->NoiseScaleSpinBox->value();
    _params.noise_w = ui->NoiseScaleWSpinBox->value();
    _params.length = ui->LengthScaleSpinBox->value();
    _params.gate = ui->GateThSpinBox->value();
    _params.decode_step = ui->MaxDecodeStepSpinBox->value();
    _params.emotion = ui->EmotionEdit->text().toStdWString();
	_params.chara_mix = charaMixData;
    _params.phs = ui->SrcTextEdit->text().toStdWString();
    std::vector<int64_t> dur;
    if(!ui->DurationEdit->text().isEmpty())
		for (const auto& i : ui->DurationEdit->text().split(' '))
		{
            if(i.isEmpty())
                continue;
			auto ii = i.toInt();
            if (ii < 1)
                ii = 1;
			dur.emplace_back(ii);
		}
    _params.durations = dur;

    std::vector<int64_t> tone;
    if(!ui->ToneEdit->text().isEmpty())
		for (const auto& i : ui->ToneEdit->text().split(' '))
		{
            if (i.isEmpty())
                continue;
			tone.emplace_back(i.toInt());
		}
    _params.tones = tone;
    _params.seed = ui->TTSParamSeedEdit->text().toInt();
    if (textCleaner.DictEnabled())
        _params.PlaceHolder = textCleaner.getPlaceholderSymbol()[0];
    return _params;
}

void TTSMainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasText())
        event->acceptProposedAction();
    else event->ignore();
}

void TTSMainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* qMimeData_drag = event->mimeData();
    QList<QUrl> urlList;
    std::vector<std::string> _texts;
    if (qMimeData_drag->hasUrls())
        urlList = qMimeData_drag->urls();
    for (const auto& i : urlList)
    {
    	const auto path = i.toLocalFile().toStdWString();
        const auto file_suffix = path.substr(path.rfind(L'.') + 1);
        if (file_suffix == L"mttsproj")
        {
            try
            {
                const auto temp = _proj.load(path, MoeVSProject::TTSProject::T_LOAD::APPEND);
                _texts.insert(_texts.end(), temp.begin(), temp.end());
            }
        	catch(std::exception& e)
            {
                QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
            }
        }
        else if (file_suffix == L"wav" || file_suffix == L"mp3" || file_suffix == L"ogg" || file_suffix == L"flac")
        {
            waveFolder.emplace_back(path);
            ui->AudioList->addItem(i.fileName());
        }
    }
    if (!_texts.empty())
        for (const auto& i : _texts)
            ui->TextListWidget->addItem(i.c_str());
}

int TTSMainWindow::saveProject(std::string& error)
{
    const auto file_path = QFileDialog::getSaveFileName(this, tr("SaveTTSProject"), to_byte_string(GetCurrentFolder()).c_str(), "MttsProj files (*.mttsproj);");
    if (file_path.isEmpty())
        return 1;
    try
	{
    	_proj.Write(file_path.toStdWString());
        return 0;
	}
    catch(std::exception& e)
    {
        error = e.what();
        return -1;
    }
}

void TTSMainWindow::initPlayer()
{
    media_player = new QMediaPlayer(this);
    media_audio_output = new QAudioOutput(this);
    media_player->setAudioOutput(media_audio_output);
    media_audio_output->setDevice(QAudioDevice());
    media_audio_output->setVolume(float(ui->VolumeSlider->value()) / 100);
    ui->PlayerProgress->setValue(0);
    ui->PlayerProgress->setMaximum(0);
    setSoundModelEnabled(false);
}

void TTSMainWindow::setSoundModelEnabled(bool condition) const
{
    ui->PlayerPlayButton->setEnabled(condition);
    ui->PlayerProgress->setEnabled(condition);
}

void TTSMainWindow::setAudio(const std::wstring& path) const
{
	wave_widget_->setWavFile(path);
    media_player->setSource(QUrl::fromLocalFile(to_byte_string(path).c_str()));
}

//*******************AudioSlotFn**********************//

void TTSMainWindow::getDurationChanged(qint64 time) {
    duration_media = int(time);
    const auto audio_dur = int(time);
    ui->PlayerProgress->setMaximum(audio_dur);
    ui->PlayerProgress->setValue(0);
    media_player->setPosition(0);
    const auto times = QString("%1:%2").arg(audio_dur / 60000, 2, 10, QChar('0')).arg((audio_dur % 60000) / 1000, 2, 10, QChar('0'));
    ui->MaxTimePlayer->setText(times);
    ui->CurTimePlayer->setText("00:00");
    if (duration_media)
        setSoundModelEnabled(true);
    else
        setSoundModelEnabled(false);
}

void TTSMainWindow::playbackPosChanged(qint64 time) const
{
    ui->PlayerProgress->setValue(int(time));
}

//*******************EventFn**********************//

void TTSMainWindow::on_GateThSlider_valueChanged(int value) const
{
    ui->GateThSpinBox->setValue(double(value) / 10000.);
}

void TTSMainWindow::on_LengthScaleSlider_valueChanged(int value) const
{
    ui->LengthScaleSpinBox->setValue(double(value) / 1000.);
}

void TTSMainWindow::on_MaxDecodeStepSlider_valueChanged(int value) const
{
    ui->MaxDecodeStepSpinBox->setValue(value);
}

void TTSMainWindow::on_NoiseScaleSlider_valueChanged(int value) const
{
    ui->NoiseScaleSpinBox->setValue(double(value) / 1000.);
}

void TTSMainWindow::on_NoiseScaleWSlider_valueChanged(int value) const
{
    ui->NoiseScaleWSpinBox->setValue(double(value) / 1000.);
}

void TTSMainWindow::on_GateThSpinBox_valueChanged(double value) const
{
    ui->GateThSlider->setValue(int(value * 10000));
}

void TTSMainWindow::on_LengthScaleSpinBox_valueChanged(double value) const
{
    ui->LengthScaleSlider->setValue(int(value * 1000));
}

void TTSMainWindow::on_MaxDecodeStepSpinBox_valueChanged(int value) const
{
    ui->MaxDecodeStepSlider->setValue(value);
}

void TTSMainWindow::on_NoiseScaleSpinBox_valueChanged(double value) const
{
    ui->NoiseScaleSlider->setValue(int(value * 1000));
}

void TTSMainWindow::on_NoiseScaleWSpinBox_valueChanged(double value) const
{
    ui->NoiseScaleWSlider->setValue(int(value * 1000));
}

void TTSMainWindow::on_ModelListRefreshButton_clicked()
{
    ui->ModelListRefreshButton->setEnabled(false);
    reloadModels();
    ui->ModelListRefreshButton->setEnabled(true);
}

void TTSMainWindow::on_LoadModelButton_clicked()
{
    ui->LoadModelButton->setEnabled(false);
    loadModel(ui->ModelComboBox->currentIndex());
    ui->LoadModelButton->setEnabled(true);
}

void TTSMainWindow::on_InferenceButton_clicked()
{
    if (!_model)
    {
        QMessageBox::warning(this, tr("TTSModelNotExistErrorTitle"), tr("TTSModelNotExistErrorText"), QMessageBox::Ok);
        return;
    }
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            auto inp = ui->InferTextEdit->toPlainText().toStdWString();
            if (!inp.empty())
            {
                std::vector<int16_t> PCMDATA;
                try
                {
                    PCMDATA = _model->Inference(inp);
                }
                catch (std::exception& e)
                {
                    _InferMsgSender.astrParamEvent(e.what());
                    _InferMsgSender.InferProcess(0, 1);
                    _InferMsgSender.ChangeInferenceStat(true);
                    return;
                }
                std::wstring OutDir;
                for (size_t i = 0; i < (1ull << 20ull); ++i)
                {
                    OutDir = GetCurrentFolder() + L"\\OutPuts\\" + std::to_wstring(i) + L".wav";
                    if (_waccess(OutDir.c_str(), 0) == -1)
                        break;
                }
                Wav(uint32_t(samplingRate), uint32_t(PCMDATA.size() * 2ull), PCMDATA.data()).Writef(OutDir);
                waveFolder.emplace_back(OutDir);
                _InferMsgSender.strParamEvent(OutDir);
            }
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void TTSMainWindow::on_CharacterMixProportion_valueChanged(double value)
{
    charaMixData[ui->CharacterComboBox->currentIndex()] = (float)value;
    float sum = 0.f;
    for (const auto& i : charaMixData)
        sum += i;
    if (sum < 0.001f)
    {
        sum = 1.f;
        charaMixData[0] = 1.f;
    }
    for (auto& i : charaMixData)
        i /= sum;
    ui->CharacterMixProportion->setValue((double)charaMixData[ui->CharacterComboBox->currentIndex()]);
}

void TTSMainWindow::on_CharacterComboBox_currentIndexChanged(int value) const
{
    if (charaMixData.empty())
        return;
    ui->CharacterMixProportion->setValue((double)charaMixData[ui->CharacterComboBox->currentIndex()]);
}

void TTSMainWindow::on_AddTextButton_clicked()
{
	auto params = dumpParamsFromControl();

    if (params.phs.empty())
    {
        QMessageBox::warning(this, tr("TTSPhsSizeErrorTitle"), tr("TTSPhsSizeErrorText"), QMessageBox::Ok);
        return;
    }

    ui->TextListWidget->addItem(to_byte_string(params.phs).c_str());
    _proj.push(std::move(params));
}

void TTSMainWindow::on_RemoveTextButton_clicked()
{
    if (!ui->TextListWidget->currentItem())
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("TTSRemoveTextButtonButtonTitle"));
    msgBox.setText(tr("TTSRemoveTextButtonButtonText"));
    msgBox.setInformativeText(tr("TTSRemoveTextButtonButtonInformativeText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("TTSRemoveTextButtonButtonButtonYes"));
    msgBox.button(QMessageBox::No)->setText(tr("TTSRemoveTextButtonButtonButtonNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
        const auto index = ui->TextListWidget->currentIndex().row();
        delete ui->TextListWidget->takeItem(index);
        _proj.erase(index);
    }
}

void TTSMainWindow::on_TextListWidget_itemSelectionChanged() const
{
    ui->SaveTextButton->setEnabled(ui->TextListWidget->currentItem() != nullptr);
}

void TTSMainWindow::on_SaveTextButton_clicked()
{
    if(ui->TextListWidget->currentItem())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("TTSSaveTextButtonButtonTitle"));
        msgBox.setText(tr("TTSSaveTextButtonButtonText"));
        msgBox.setInformativeText(tr("TTSSaveTextButtonButtonInformativeText"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.button(QMessageBox::Yes)->setText(tr("TTSSaveTextButtonYes"));
        msgBox.button(QMessageBox::No)->setText(tr("TTSSaveTextButtonNo"));
        msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
        const auto ret = msgBox.exec();
        if (ret == QMessageBox::No)
            return;
        const auto idx = ui->TextListWidget->currentRow();
    	auto params = dumpParamsFromControl();
        if (params.phs.empty())
        {
            QMessageBox::warning(this, tr("TTSPhsSizeErrorTitle"), tr("TTSPhsSizeErrorText"), QMessageBox::Ok);
            return;
        }
        ui->TextListWidget->currentItem()->setText(to_byte_string(params.phs).c_str());
        _proj[idx] = std::move(params);
    }
}

void TTSMainWindow::on_EditTextButton_clicked()
{
    if (ui->TextListWidget->currentItem())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("TTSEditTextButtonButtonTitle"));
        msgBox.setText(tr("TTSEditTextButtonButtonText"));
        msgBox.setInformativeText(tr("TTSEditTextButtonButtonInformativeText"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.button(QMessageBox::Yes)->setText(tr("TTSEditTextButtonYes"));
        msgBox.button(QMessageBox::No)->setText(tr("TTSEditTextButtonNo"));
        msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
        const auto ret = msgBox.exec();
        if (ret == QMessageBox::No)
            return;
        const auto idx = ui->TextListWidget->currentRow();
        const auto& params_ = _proj[idx];
        setParamsControlValue(params_);
    }
}

void TTSMainWindow::on_TTSInferenceCleanerButton_clicked()
{
    const std::wregex changeLineSymb(L"\r");
	if(!_model)
	{
        QMessageBox::warning(this, tr("TTSModelNotExistErrorTitle"), tr("TTSModelNotExistErrorText"), QMessageBox::Ok);
        return;
	}
    const std::wstring EndString = _model->getEndString();
    if(textCleaner.G2pEnabled())
    {
        auto input = ui->InferTextEdit->toPlainText().toStdWString() + L'\n';
        input = std::regex_replace(input, changeLineSymb, L"\n");
        std::wstring output;
        output.reserve(input.size());
        auto fit = input.find(L'\n');
        while (fit != std::wstring::npos)
        {
            std::wstring tmp = input.substr(0, fit);
            const size_t fit_t = tmp.rfind(EndString);
            if (fit_t != std::wstring::npos)
            {
                output += tmp.substr(0, fit_t + 1);
                tmp = tmp.substr(fit_t + 1);
            }
            tmp = textCleaner.G2p(tmp);
            output += tmp + L'\n';
            input = input.substr(fit + 1);
            fit = input.find(L'\n');
        }
        ui->InferTextEdit->setText(QString::fromStdWString(output));
    }else
    {
        QMessageBox::warning(this, tr("TTSCleanerNotExistErrorTitle"), tr("TTSCleanerNotExistErrorText"), QMessageBox::Ok);
    }
}

void TTSMainWindow::on_TextCleaner_clicked()
{
    if (textCleaner.G2pEnabled())
    {
        const auto input = ui->SrcTextEdit->text().toStdWString() + L'\n';
        if (input.size() < 2)
            return;
        ui->SrcTextEdit->setText(QString::fromStdWString(textCleaner.G2p(input)));
    }
    else
    {
        QMessageBox::warning(this, tr("TTSCleanerNotExistErrorTitle"), tr("TTSCleanerNotExistErrorText"), QMessageBox::Ok);
    }
}

void TTSMainWindow::on_ClearTextListButton_clicked()
{
    if (!ui->TextListWidget->count())
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("TTSClearTextListTitle"));
    msgBox.setText(tr("TTSClearTextListText"));
    msgBox.setInformativeText(tr("TTSClearTextListInformativeText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("TTSClearTextListYes"));
    msgBox.button(QMessageBox::No)->setText(tr("TTSClearTextListNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
        std::string error_txt;
        if(saveProject(error_txt) != -1)
        {
	        ui->TextListWidget->clear();
        	_proj.clear();
        }
        else
        {
            QMessageBox::warning(this, "ERROR", error_txt.c_str(), QMessageBox::Ok);
        }
    }
}

void TTSMainWindow::on_BatchedTTSButton_clicked()
{
    if (!_model)
    {
        QMessageBox::warning(this, tr("TTSModelNotExistErrorTitle"), tr("TTSModelNotExistErrorText"), QMessageBox::Ok);
        return;
    }
    if (_proj.data().empty())
        return;
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            auto _params_all = _proj.data();
            for (auto& params : _params_all)
            {
                if (ui->CharacterCheckBox->checkState() != Qt::Checked)
                {
                    params.chara_mix.clear();
                    if (n_speakers)
                        params.chara = ui->CharacterComboBox->currentIndex();
                    else
                        params.chara = 0;
                }
                else
                {
	                if (params.chara_mix.size() > size_t(n_speakers))
	                	params.chara_mix.resize(n_speakers);
                    InferClass::OnnxModule::LinearCombination(params.chara_mix, 1.f);
                }
                if(textCleaner.DictEnabled())
                {
                    if (ui->UseDictCheckBox->checkState() == Qt::Checked)
                        params.phs = textCleaner.DictReplaceGetStr(params.phs, std::wstring(L"") + params.PlaceHolder, true);
                    else
                    {
	                    params.phs = textCleaner.DictReplaceGetStr(params.phs, std::wstring(L"") + params.PlaceHolder, false);
                        params.PlaceHolder = 0;
                    }
                }
                else
                    params.PlaceHolder = 0;
            }
            try
            {
                const auto paths = _model->Inference(_params_all);
                for (const auto& OutDir : paths)
                {
                    waveFolder.emplace_back(OutDir);
                    _InferMsgSender.strParamEvent(OutDir);
                }
            }
			catch(std::exception& e)
			{
                _InferMsgSender.astrParamEvent(e.what());
                _InferMsgSender.InferProcess(0, 1);
                _InferMsgSender.ChangeInferenceStat(true);
                return;
			}
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void TTSMainWindow::on_TTSInferOnceButton_clicked()
{
    if (!_model)
    {
        QMessageBox::warning(this, tr("TTSModelNotExistErrorTitle"), tr("TTSModelNotExistErrorText"), QMessageBox::Ok);
        return;
    }
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            if (ui->TextListWidget->currentItem())
            {
                std::vector<int16_t> PCMDATA;
                try
                {
                    auto params = _proj[ui->TextListWidget->currentRow()];
                    if (ui->CharacterCheckBox->checkState() != Qt::Checked)
                    {
                        params.chara_mix.clear();
                        if (n_speakers)
                            params.chara = ui->CharacterComboBox->currentIndex();
                        else
                            params.chara = 0;
                    }
                    else
                    {
	                    if (params.chara_mix.size() > size_t(n_speakers))
	                    	params.chara_mix.resize(n_speakers);
                        InferClass::OnnxModule::LinearCombination(_cur_params.chara_mix, 1.f);
                    }
                    if (textCleaner.DictEnabled())
                    {
                        if (ui->UseDictCheckBox->checkState() == Qt::Checked)
                            params.phs = textCleaner.DictReplaceGetStr(params.phs, std::wstring(L"") + params.PlaceHolder, true);
                        else
                        {
                            params.phs = textCleaner.DictReplaceGetStr(params.phs, std::wstring(L"") + params.PlaceHolder, false);
                            params.PlaceHolder = 0;
                        }
                    }
                    else
                        params.PlaceHolder = 0;
                    PCMDATA = _model->Inference(params);
                }
                catch (std::exception& e)
                {
                    _InferMsgSender.astrParamEvent(e.what());
                    _InferMsgSender.InferProcess(0, 1);
                    _InferMsgSender.ChangeInferenceStat(true);
                    return;
                }
                std::wstring OutDir;
                for (size_t i = 0; i < (1ull << 20ull); ++i)
                {
                    OutDir = GetCurrentFolder() + L"\\OutPuts\\" + std::to_wstring(i) + L".wav";
                    if (_waccess(OutDir.c_str(), 0) == -1)
                        break;
                }
                Wav(uint32_t(samplingRate), uint32_t(PCMDATA.size() * 2ull), PCMDATA.data()).Writef(OutDir);
                waveFolder.emplace_back(OutDir);
                _InferMsgSender.strParamEvent(OutDir);
            }
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void TTSMainWindow::on_TTSInferCur_clicked()
{
    if (!_model)
    {
        QMessageBox::warning(this, tr("TTSModelNotExistErrorTitle"), tr("TTSModelNotExistErrorText"), QMessageBox::Ok);
        return;
    }
    _cur_params = dumpParamsFromControl();
    if (_cur_params.phs.empty())
    {
        QMessageBox::warning(this, tr("TTSPhsSizeErrorTitle"), tr("TTSPhsSizeErrorText"), QMessageBox::Ok);
        return;
    }
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            std::vector<int16_t> PCMDATA;
            try
            {
                if (ui->CharacterCheckBox->checkState() != Qt::Checked)
                {
                    _cur_params.chara_mix.clear();
                    if (n_speakers)
                        _cur_params.chara = ui->CharacterComboBox->currentIndex();
                    else
                        _cur_params.chara = 0;
                }
                else
                {
	                if (_cur_params.chara_mix.size() > size_t(n_speakers))
	                	_cur_params.chara_mix.resize(n_speakers);
                    InferClass::OnnxModule::LinearCombination(_cur_params.chara_mix, 1.f);
                }
                if (textCleaner.DictEnabled())
                {
                    if (ui->UseDictCheckBox->checkState() == Qt::Checked)
                        _cur_params.phs = textCleaner.DictReplaceGetStr(_cur_params.phs, std::wstring(L"") + _cur_params.PlaceHolder, true);
                    else
                    {
                        _cur_params.phs = textCleaner.DictReplaceGetStr(_cur_params.phs, std::wstring(L"") + _cur_params.PlaceHolder, false);
                        _cur_params.PlaceHolder = 0;
                    }
                }
                else
                    _cur_params.PlaceHolder = 0;
                PCMDATA = _model->Inference(_cur_params);
            }
            catch (std::exception& e)
            {
                _InferMsgSender.astrParamEvent(e.what());
                _InferMsgSender.InferProcess(0, 1);
                _InferMsgSender.ChangeInferenceStat(true);
                return;
            }
            std::wstring OutDir;
            for (size_t i = 0; i < (1ull << 20ull); ++i)
            {
                OutDir = GetCurrentFolder() + L"\\OutPuts\\" + std::to_wstring(i) + L".wav";
                if (_waccess(OutDir.c_str(), 0) == -1)
                    break;
            }
            Wav(uint32_t(samplingRate), uint32_t(PCMDATA.size() * 2ull), PCMDATA.data()).Writef(OutDir);
            waveFolder.emplace_back(OutDir);
            _InferMsgSender.strParamEvent(OutDir);
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void TTSMainWindow::on_AudioList_itemDoubleClicked(QListWidgetItem* item)
{
    if (item)
    {
        const auto idx = ui->AudioList->row(item);
        setAudio(waveFolder[idx]);
        cur_media = idx;
    }
}

void TTSMainWindow::on_TextListWidget_itemDoubleClicked(QListWidgetItem* item)
{
    if (item)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("TTSEditTextButtonButtonTitle"));
        msgBox.setText(tr("TTSEditTextButtonButtonText"));
        msgBox.setInformativeText(tr("TTSEditTextButtonButtonInformativeText"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.button(QMessageBox::Yes)->setText(tr("TTSEditTextButtonYes"));
        msgBox.button(QMessageBox::No)->setText(tr("TTSEditTextButtonNo"));
        msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
        const auto ret = msgBox.exec();
        if (ret == QMessageBox::No)
            return;
        const auto idx = ui->TextListWidget->row(item);
        const auto& params_ = _proj[idx];
        setParamsControlValue(params_);
    }
}

void TTSMainWindow::on_OpenProjectButton_clicked()
{
    const auto file_path = QFileDialog::getOpenFileName(this, tr("SaveTTSProject"), to_byte_string(GetCurrentFolder()).c_str(), "MttsProj files (*.mttsproj);");
    if (file_path.isEmpty())
        return;
    const auto _texts = _proj.load(file_path.toStdWString(), MoeVSProject::TTSProject::T_LOAD::APPEND);
    if (!_texts.empty())
        for (const auto& i : _texts)
            ui->TextListWidget->addItem(i.c_str());
}

void TTSMainWindow::on_SaveProjectButton_clicked()
{
    if (_proj.data().empty())
        return;
    std::string error_txt;
    if (saveProject(error_txt) == -1)
        QMessageBox::warning(this, "ERROR", error_txt.c_str(), QMessageBox::Ok);
}

void TTSMainWindow::on_TTSOpenEditor_clicked()
{
    editor.show();
}

void TTSMainWindow::on_DictComboBox_currentIndexChanged(int value)
{
    if (value)
        textCleaner.loadDict(DictPaths[value - 1]);
    else
        textCleaner.unloadDict();
    ui->UseDictCheckBox->setCheckState(Qt::Unchecked);
    ui->UseDictCheckBox->setEnabled(textCleaner.DictEnabled());
}

void TTSMainWindow::on_G2pComboBox_currentIndexChanged(int value)
{
    if (value)
        textCleaner.loadG2p(CleanerPaths[value - 1]);
    else
        textCleaner.unloadG2p();

    ui->TextCleaner->setEnabled(textCleaner.G2pEnabled());
    ui->TTSInferenceCleanerButton->setEnabled(textCleaner.G2pEnabled());
}

/*********Player**********/

void TTSMainWindow::on_ClearListButton_clicked()
{
    if (!ui->AudioList->count())
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("TTSAudioListClearTitle"));
    msgBox.setText(tr("TTSAudioListClearText"));
    msgBox.setInformativeText(tr("TTSAudioListInformativeClearText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("TTSAudioListClearYes"));
    msgBox.button(QMessageBox::No)->setText(tr("TTSAudioListClearNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
        ui->AudioList->clear();
        waveFolder.clear();
        setAudio(L"");
        cur_media = -1;
    }
}

void TTSMainWindow::on_DrawMelSpecButton_clicked()
{
    std::thread MelThread([&]()
        {
            _InferMsgSender.ChangeAnyStat(false);
            if (ui->AudioList->currentItem())
                if (!waveFolder.empty())
                    ui->MelWidget->setWavFile(waveFolder[ui->AudioList->currentRow()]);
            _InferMsgSender.mPaintEventSender();
            _InferMsgSender.ChangeAnyStat(true);
        });
    MelThread.detach();
}

void TTSMainWindow::on_SendToPlayerButton_clicked()
{
    if (ui->AudioList->currentItem())
    {
        const auto idx = ui->AudioList->currentRow();
        setAudio(waveFolder[idx]);
        cur_media = idx;
    }
}

void TTSMainWindow::on_DeleteSelectedAudioButton_clicked()
{
    if (!ui->AudioList->currentItem())
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("TTSDeleteSelectedAudioButtonTitle"));
    msgBox.setText(tr("TTSDeleteSelectedAudioButtonText"));
    msgBox.setInformativeText(tr("TTSDeleteSelectedAudioButtonInformativeText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("TTSDeleteSelectedAudioButtonButtonYes"));
    msgBox.button(QMessageBox::No)->setText(tr("TTSDeleteSelectedAudioButtonButtonNo"));
    msgBox.setWindowFlag(Qt::WindowStaysOnTopHint);
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
        const auto index = ui->AudioList->currentIndex().row();
        delete ui->AudioList->takeItem(index);
        waveFolder.erase(waveFolder.begin() + index);
        if (cur_media != -1ull && cur_media >= size_t(index))
        {
            if (cur_media)
                --cur_media;
            if (waveFolder.empty())
                setAudio(L"");
            else
                setAudio(waveFolder[cur_media]);
        }
        if (index != 0)
            ui->AudioList->setCurrentRow(index - 1);
        if (waveFolder.empty())
            setAudio(L"");
    }
}

void TTSMainWindow::on_LastAudioButton_clicked()
{
    if (waveFolder.empty())
    {
        cur_media = -1;
        return;
    }
    if (cur_media == 0 || cur_media == -1ull)
    {
        cur_media = waveFolder.size() - 1;
        setAudio(waveFolder[cur_media]);
    }
    else
    {
        --cur_media;
        setAudio(waveFolder[cur_media]);
    }
    ui->AudioList->setCurrentRow(int(cur_media));
}

void TTSMainWindow::on_NextAudioButton_clicked()
{
    if (waveFolder.empty())
    {
        cur_media = -1;
        return;
    }
    if (cur_media == waveFolder.size() - 1)
    {
        cur_media = 0;
        setAudio(waveFolder[cur_media]);
    }
    else
    {
        ++cur_media;
        setAudio(waveFolder[cur_media]);
    }
    ui->AudioList->setCurrentRow(int(cur_media));
}

void TTSMainWindow::on_PlayerPlayButton_clicked() const
{
    if (!duration_media)
        return;
    switch (media_player->playbackState())
    {
    case QMediaPlayer::PlayingState:
        media_player->pause();
        break;
    case QMediaPlayer::PausedState:
        media_player->play();
        break;
    case QMediaPlayer::StoppedState:
        media_player->play();
        break;
    }
}

void TTSMainWindow::on_MuteButton_clicked() const
{
    media_audio_output->setVolume(0.f);
    ui->VolumeSlider->setValue(0);
    //TODO
}

void TTSMainWindow::on_PlayerProgress_valueChanged(int value) const
{
    if (!duration_media)
        return;
    if (value == duration_media) {
        ui->PlayerProgress->setValue(0);
        media_player->stop();
        media_player->setPosition(0);
        return;
    }
    media_player->setPosition(value);
}

void TTSMainWindow::on_VolumeSlider_valueChanged(int value) const
{
    media_audio_output->setVolume(float(value) / 100);
    if (!value)
    {
        //TODO
    }
}

/*************Other**************/

void TTSMainWindow::InferenceProcessChanged(size_t cur, size_t all) const
{
    if (!cur)
        ui->InferProgressBar->setMaximum((int)all);
    ui->InferProgressBar->setValue((int)cur);
}

void TTSMainWindow::InferenceStatChanged(bool condition) const
{
    SetInferenceEnabled(condition);
    SetModelSelectEnabled(condition);
    ui->DrawMelSpecButton->setEnabled(condition);
}

void TTSMainWindow::InsertFilePaths(std::wstring _str) const
{
    ui->AudioList->addItem(to_byte_string(_str.substr(_str.rfind(L'\\') + 1)).c_str());
}

void TTSMainWindow::ErrorMsgBoxEvent(std::string _str)
{
    QMessageBox::warning(this, "ERROR", _str.c_str(), QMessageBox::Ok);
}

/**********Win32Only************/
#ifdef WIN32
void TTSMainWindow::openModelFolder()
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    si.cb = sizeof(STARTUPINFO);
    CreateProcess(nullptr,
        const_cast<LPWSTR>((L"Explorer.exe " + GetCurrentFolder() + L"\\Models").c_str()),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void TTSMainWindow::on_ModelListDirButton_clicked()
{
    openModelFolder();
}
#endif