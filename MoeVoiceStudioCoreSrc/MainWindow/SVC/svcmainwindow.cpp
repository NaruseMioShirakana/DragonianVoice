#include "svcmainwindow.h"
#include "ui_svcmainwindow.h"
#include <VitsSvc.hpp>
#include <DiffSvc.hpp>

SVCMainWindow::SVCMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SVCMainWindow),
    exit_callback(_callback)
{
    ui->setupUi(this);
    initSetup();
    connect(media_player, SIGNAL(durationChanged(qint64)), this, SLOT(getDurationChanged(qint64)));
    connect(media_player, SIGNAL(positionChanged(qint64)), this, SLOT(playbackPosChanged(qint64)));
    connect(&_InferMsgSender, SIGNAL(onProcessBarChanging(size_t, size_t)), this, SLOT(InferenceProcessChanged(size_t, size_t)));
    connect(&_InferMsgSender, SIGNAL(onAnyStatChanging(bool)), this, SLOT(InferenceStatChanged(bool)));
    connect(&_InferMsgSender, SIGNAL(astrParamedEvent(std::string)), this, SLOT(ErrorMsgBoxEvent(std::string)));
    connect(&_InferMsgSender, SIGNAL(strParamedEvent(std::wstring)), this, SLOT(InsertFilePaths(std::wstring)));
    const QDir q_output_dir(to_byte_string(GetCurrentFolder() + L"/outputs").c_str());
    QStringList q_filename;
    q_filename << "*.wav" << "*.mp3" << "*.flac" << "*.ogg";
    QStringList q_output_results = q_output_dir.entryList(q_filename, QDir::Files | QDir::Readable, QDir::Name);
    for(const auto& i : q_output_results)
    {
	    ui->SvcPlayListWidget->addItem(i);
        _audioFolder.emplace_back(GetCurrentFolder() + L"/outputs/" + i.toStdWString());
    }
}

SVCMainWindow::~SVCMainWindow()
{
    delete ui;
}

void SVCMainWindow::reloadModels()
{
    modelsClear();
    ui->SvcModelSelector->setEnabled(false);
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
    ui->SvcModelSelector->addItem("None");
    if (-1 != handle) {
        do
        {
            std::string modInfo, modInfoAll;
            std::ifstream modfile((current_path + L"\\" + file_info.name).c_str());
            while (std::getline(modfile, modInfo))
                modInfoAll += modInfo;
            modfile.close();
            MJson modConfigJson;
            modConfigJson.Parse(modInfoAll);
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

            if (Type != "SoVits" && Type != "RVC" && Type != "DiffSvc")
                continue;
            auto _tmpText = Type + ":" + Name;
            ui->SvcModelSelector->addItem(_tmpText.c_str());
            _models.emplace_back(std::move(modConfigJson));
        } while (!_wfindnext(handle, &file_info));
    }
	ui->SvcModelSelector->setEnabled(!_models.empty());
    ui->vcLoadModelButton->setEnabled(!_models.empty());
}

void SVCMainWindow::SetInferenceEnabled(bool condition) const
{
    ui->SvcProjectInferCurPushButton->setEnabled(condition);
    ui->SvcInferCurSlice->setEnabled(condition);
    ui->SvcMoeSSInfer->setEnabled(condition);
}

void SVCMainWindow::initSetup()
{
    setAcceptDrops(true);
    SetInferenceEnabled(false);
    ui->SvcCharacterSelector->setEnabled(false);
    initPlayer();
    reloadModels();
}

void SVCMainWindow::setSoundModelEnabled(bool condition) const
{
    ui->SvcPlayerPlayPosHorizontalSlider->setEnabled(condition);
    ui->SvcPlayerPlayButton->setEnabled(condition);
}

void SVCMainWindow::initPlayer()
{
    media_player = new QMediaPlayer(this);
    media_audio_output = new QAudioOutput(this);
    media_player->setAudioOutput(media_audio_output);
    media_audio_output->setDevice(QAudioDevice());
    media_audio_output->setVolume(float(ui->SvcPlayerVolumeSlider->value()) / 100);
    ui->SvcPlayerPlayPosHorizontalSlider->setValue(0);
    ui->SvcPlayerPlayPosHorizontalSlider->setMaximum(0);
    setSoundModelEnabled(false);
}

void SVCMainWindow::modelsClear()
{
    unloadModel();
    ui->SvcModelSelector->clear();
    _models.clear();
}

void SVCMainWindow::unloadModel()
{
    SetInferenceEnabled(false);
    delete _model;
    _model = nullptr;
    ui->SvcCharacterSelector->clear();
    ui->SvcCharacterSelector->setEnabled(false);
}

void SVCMainWindow::loadModel(size_t idx)
{
    if (idx == cur_model_index)
        return;
    unloadModel();
    if (idx == 0)
    {
        ui->SvcEditorWidget->setOpenAudioEnabled(true);
        cur_model_index = 0;
        return;
    }
    const auto index_model = idx - 1;
    const auto& Config = _models[index_model];
    try
    {
        const std::string _type_model = Config["Type"].GetString();
        if (_type_model == "DiffSvc")
        {
            _model = dynamic_cast<InferClass::SVC*>(new InferClass::DiffusionSvc(Config, BarCallback, TTSParamCallback, __MOESS_DEVICE));
        }
        else if (_type_model == "SoVits" || _type_model == "RVC")
        {
            _model = dynamic_cast<InferClass::SVC*>(new InferClass::VitsSvc(Config, BarCallback, TTSParamCallback, __MOESS_DEVICE));
        }
        else
        {
            QMessageBox::warning(this, "ERROR", "Unsupported model", QMessageBox::Ok);
            ui->SvcModelSelector->setCurrentIndex(0);
            cur_model_index = 0;
            return;
        }
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
        ui->SvcModelSelector->setCurrentIndex(0);
        cur_model_index = 0;
        return;
    }
    samplingRate = _model->GetSamplingRate();
    _speakers.clear();
    if (Config.HasMember("Characters"))
    {
        if (Config["Characters"].IsArray() && !Config["Characters"].Empty())
        {
            ui->SvcCharacterSelector->setEnabled(true);
            for (auto& it : Config["Characters"].GetArray())
            {
	            ui->SvcCharacterSelector->addItem(it.GetString().c_str());
                _speakers.emplace_back(it.GetString());
            }
            n_speakers = int(Config["Characters"].Size());
        }
    }
    ui->SvcEditorWidget->setOpenAudioEnabled(false);
    cur_model_index = idx;
    SetInferenceEnabled(true);
}

void SVCMainWindow::SetModelSelectEnabled(bool condition) const
{
    ui->vcLoadModelButton->setEnabled(condition);
    ui->vcReLoadModelButton->setEnabled(condition);
}

//*******************EventsFn*************************//

void SVCMainWindow::closeEvent(QCloseEvent* event)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("SvcQuitTitle"));
    msgBox.setText(tr("SvcQuitText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("SvcQuitYes"));
    msgBox.button(QMessageBox::No)->setText(tr("SvcQuitNo"));
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
        event->accept();
    else
        event->ignore();
}

void SVCMainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_S)
    {
        if (event->modifiers() == Qt::ControlModifier && saveEnabled)
        {
            on_SvcSaveSegment_clicked();
            return;
        }
        if (event->modifiers() == Qt::ControlModifier | Qt::AltModifier && saveEnabled)
        {
            on_SvcSaveSegment_clicked();
            on_actionSvcMainSaveProject_triggered();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

//*******************AudioSlotFn**********************//

void SVCMainWindow::getDurationChanged(qint64 time) {
    duration_media = int(time);
    const auto audio_dur = int(time);
    ui->SvcPlayerPlayPosHorizontalSlider->setMaximum(audio_dur);
    ui->SvcPlayerPlayPosHorizontalSlider->setValue(0);
    media_player->setPosition(0);
    const auto times = QString("%1:%2").arg(audio_dur / 60000, 2, 10, QChar('0')).arg((audio_dur % 60000) / 1000, 2, 10, QChar('0'));
    ui->SvcPlayerTotalTimeLabel->setText(times);
    ui->SvcPlayerCurTimeLabel->setText("00:00");
    if (duration_media)
        setSoundModelEnabled(true);
    else
        setSoundModelEnabled(false);
}

void SVCMainWindow::playbackPosChanged(qint64 time) const
{
    const auto times = QString("%1:%2").arg(time / 60000, 2, 10, QChar('0')).arg((time % 60000) / 1000, 2, 10, QChar('0'));
    ui->SvcPlayerCurTimeLabel->setText(times);
    ui->SvcPlayerPlayPosHorizontalSlider->setValue(int(time));
}

//******************ButtonSlotFn**********************//

#ifdef WIN32
void SVCMainWindow::on_SvcOpenModelFolderButton_clicked()
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
#endif

void SVCMainWindow::on_vcLoadModelButton_clicked()
{
    ui->vcLoadModelButton->setEnabled(false);
    loadModel(ui->SvcModelSelector->currentIndex());
    ui->vcLoadModelButton->setEnabled(true);
}

void SVCMainWindow::on_vcReLoadModelButton_clicked()
{
    ui->vcReLoadModelButton->setEnabled(false);
    reloadModels();
    ui->vcReLoadModelButton->setEnabled(true);
}

void SVCMainWindow::on_SvcProjectDelPushButton_clicked()
{
    if (!ui->SvcProjectListWidget->currentItem())
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("SvcDeleteAudioTitle"));
    msgBox.setText(tr("SvcDeleteAudioText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("SvcDeleteAudioYes"));
    msgBox.button(QMessageBox::No)->setText(tr("SvcDeleteAudioNo"));
    const auto ret = msgBox.exec();
    if (ret == QMessageBox::Yes)
    {
        const auto index = ui->SvcProjectListWidget->currentIndex().row();
        delete ui->SvcProjectListWidget->takeItem(index);
        if (_curAudio && _curAudio == _Projects.data() + index)
        {
            _curAudio = nullptr;
            cur_slice_index = -1;
            ui->SvcSegmentListWidget->clear();
            ui->SvcEditorWidget->clearPlots();
            ui->SvcSegmentListWidget->clear();
        }
        const bool replace_stat = _curAudio && _curAudio > _Projects.data() + index;
        const auto idx = (_curAudio - _Projects.data()) - 1;
        _Projects.erase(_Projects.begin() + index);
        if (replace_stat)
            _curAudio = _Projects.data() + idx;
    }
}

void SVCMainWindow::on_SvcProjectInferCurPushButton_clicked()
{
    if(!_model)
    {
        QMessageBox::warning(this, tr("SvcPleaseLoadModelTitle"), tr("SvcPleaseLoadModelText"), QMessageBox::Ok);
        return;
    }
    if(!_curAudio)
    {
        QMessageBox::warning(this, tr("SvcPleaseSelectAParamTitle"), tr("SvcPleaseSelectAParamText"), QMessageBox::Ok);
        return;
    }
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            std::vector<int16_t> PCMDATA;
            try
            {
                PCMDATA = _model->InferCurAudio(*_curAudio);
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
                OutDir = GetCurrentFolder() + L"/OutPuts/" + _curAudio->paths + L'_' + std::to_wstring(i) + L".wav";
                if (_waccess(OutDir.c_str(), 0) == -1)
                    break;
            }
            Wav(uint32_t(samplingRate), uint32_t(PCMDATA.size() * 2ull), PCMDATA.data()).Writef(OutDir);
            _audioFolder.emplace_back(OutDir);
            _InferMsgSender.strParamEvent(OutDir.substr(OutDir.rfind(PathSeparator) + 1));
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void SVCMainWindow::on_SvcInferCurSlice_clicked()
{
    if (!_model)
    {
        QMessageBox::warning(this, tr("SvcPleaseLoadModelTitle"), tr("SvcPleaseLoadModelText"), QMessageBox::Ok);
        return;
    }
    if (!_curAudio || cur_slice_index == -1ull)
    {
        QMessageBox::warning(this, tr("SvcPleaseSelectAParamTitle"), tr("SvcPleaseSelectAParamText"), QMessageBox::Ok);
        return;
    }
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            std::vector<int16_t> PCMDATA;
            try
            {
                MoeVSProject::Params _inputParams;
                _inputParams.Speaker = {_curAudio->Speaker[cur_slice_index]};
                _inputParams.F0 = { _curAudio->F0[cur_slice_index] };
                _inputParams.Volume = { _curAudio->Volume[cur_slice_index] };
                _inputParams.OrgAudio = { _curAudio->OrgAudio[cur_slice_index] };
                _inputParams.OrgLen = { _curAudio->OrgLen[cur_slice_index] };
                _inputParams.symbolb = { _curAudio->symbolb[cur_slice_index] };
                _inputParams.paths = _curAudio->paths;
                PCMDATA = _model->InferCurAudio(_inputParams);
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
                OutDir = GetCurrentFolder() + L"/OutPuts/" + _curAudio->paths + L'_' + std::to_wstring(i) + L".wav";
                if (_waccess(OutDir.c_str(), 0) == -1)
                    break;
            }
            Wav(uint32_t(samplingRate), uint32_t(PCMDATA.size() * 2ull), PCMDATA.data()).Writef(OutDir);
            _audioFolder.emplace_back(OutDir);
            _InferMsgSender.strParamEvent(OutDir.substr(OutDir.rfind(PathSeparator) + 1));
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void SVCMainWindow::on_SvcMoeSSInfer_clicked()
{
    if (!_model)
    {
        QMessageBox::warning(this, tr("SvcPleaseLoadModelTitle"), tr("SvcPleaseLoadModelText"), QMessageBox::Ok);
        return;
    }
    std::thread Infer([&]()
        {
            _InferMsgSender.ChangeInferenceStat(false);
            std::vector<int16_t> PCMDATA;
            try
            {
                std::wstring inp;
                PCMDATA = _model->Inference(inp);
            }
            catch (std::exception& e)
            {
                _InferMsgSender.astrParamEvent(e.what());
                _InferMsgSender.InferProcess(0, 1);
                _InferMsgSender.ChangeInferenceStat(true);
                return;
            }
            if (!PCMDATA.empty())
            {
                std::wstring OutDir;
                for (size_t i = 0; i < (1ull << 20ull); ++i)
                {
                    OutDir = GetCurrentFolder() + L"/OutPuts/MoeSS_Out_Audio" + L'_' + std::to_wstring(i) + L".wav";
                    if (_waccess(OutDir.c_str(), 0) == -1)
                        break;
                }
                Wav(uint32_t(samplingRate), uint32_t(PCMDATA.size() * 2ull), PCMDATA.data()).Writef(OutDir);
                _audioFolder.emplace_back(OutDir);
                _InferMsgSender.strParamEvent(OutDir.substr(OutDir.rfind(PathSeparator) + 1));
            }
            _InferMsgSender.ChangeInferenceStat(true);
            _InferMsgSender.InferProcess(0, 1);
        });
    Infer.detach();
}

void SVCMainWindow::on_SvcPlayerMuteButton_clicked() const
{
    media_audio_output->setVolume(0.f);
    ui->SvcPlayerVolumeSlider->setValue(0);
    //TODO
}

void SVCMainWindow::on_SvcPlayerPlayButton_clicked() const
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

void SVCMainWindow::on_SvcSaveSegment_clicked() const
{
    if (_curAudio && cur_slice_index != -1ull)
    {
        auto _datas = ui->SvcEditorWidget->getData();
        _curAudio->F0[cur_slice_index] = std::move(_datas.F0);
        _curAudio->Volume[cur_slice_index] = std::move(_datas.Volume);
        _curAudio->Speaker[cur_slice_index] = std::move(_datas.Speaker);//
    }
}

//*********************clickedFn**********************//

void SVCMainWindow::on_SvcProjectListWidget_itemDoubleClicked(QListWidgetItem* item)
{
    if (item)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("SvcEditorChangeAudioHeader"));
        msgBox.setText(tr("SvcEditorChangeAudioText"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.button(QMessageBox::Yes)->setText(tr("SvcEditorChangeAudioButtonYes"));
        msgBox.button(QMessageBox::No)->setText(tr("SvcEditorChangeAudioButtonNo"));
        if (_curAudio && msgBox.exec() == QMessageBox::No)
            return;
        const auto idx = ui->SvcProjectListWidget->row(item);
        cur_slice_index = -1;
        _curAudio = &_Projects[idx];
        ui->SvcEditorWidget->clearPlots();
        ui->SvcSegmentListWidget->clear();
        for (size_t i = 0; i < _curAudio->F0.size(); ++i)
            ui->SvcSegmentListWidget->addItem(("Segment" + std::to_string(i)).c_str());
        for (size_t i = 0; i < _curAudio->F0.size(); ++i)
            if (!_curAudio->symbolb[i])
                ui->SvcSegmentListWidget->item(int(i))->setHidden(true);
    }
}

std::vector<float> ConstructSpkMixData(size_t n_speaker, size_t Length)
{
    if (n_speaker < 2)
        return {};
    std::vector<float> _LenData(n_speaker, 0.0), rtn_vector;
    _LenData[0] = 1.0;
    rtn_vector.reserve(n_speaker * Length);
    for (size_t i = 0; i < Length; ++i)
        rtn_vector.insert(rtn_vector.end(), _LenData.begin(), _LenData.end());
    return rtn_vector;
}

void SVCMainWindow::on_SvcSegmentListWidget_itemDoubleClicked(QListWidgetItem* item)
{
    if (item && _curAudio)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("SvcEditorChangeSegmentHeader"));
        msgBox.setText(tr("SvcEditorChangeSegmentText"));
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Save | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.button(QMessageBox::Yes)->setText(tr("SvcEditorChangeSegmentButtonYes"));
        msgBox.button(QMessageBox::No)->setText(tr("SvcEditorChangeSegmentButtonNo"));
        msgBox.button(QMessageBox::Save)->setText(tr("SvcEditorChangeSegmentButtonSave"));
        if (cur_slice_index != -1ull)
        {
            const auto ret = msgBox.exec();
            if (ret == QMessageBox::No)
	        	return;
            if(ret == QMessageBox::Save)
            {
                auto _datas = ui->SvcEditorWidget->getData();
                _curAudio->F0[cur_slice_index] = std::move(_datas.F0);
                _curAudio->Volume[cur_slice_index] = std::move(_datas.Volume);
                _curAudio->Speaker[cur_slice_index] = std::move(_datas.Speaker);//
            }
        }
        cur_slice_index = ui->SvcSegmentListWidget->row(item);
        if (n_speakers > 1 &&
            (_curAudio->Speaker[cur_slice_index].empty() ||
            _curAudio->Speaker[cur_slice_index].size() / n_speakers != _curAudio->F0[cur_slice_index].size()))
            _curAudio->Speaker[cur_slice_index] = ConstructSpkMixData(n_speakers, _curAudio->Volume[cur_slice_index].size());
        ui->SvcEditorWidget->setData(_curAudio->F0[cur_slice_index], 
            _curAudio->Volume[cur_slice_index],
            _curAudio->Speaker[cur_slice_index],
            _curAudio->OrgAudio[cur_slice_index],
            _speakers, 48000);
    }
}

/*************Other**************/

void SVCMainWindow::InferenceProcessChanged(size_t cur, size_t all) const
{
    if (!cur)
        ui->SvcInferProgressBar->setMaximum((int)all);
    ui->SvcInferProgressBar->setValue((int)cur);
}

void SVCMainWindow::InferenceStatChanged(bool condition)
{
    SetInferenceEnabled(condition);
    SetModelSelectEnabled(condition);
    ui->SvcProjectDelPushButton->setEnabled(condition);
    ui->SvcSaveSegment->setEnabled(condition);
    ui->actionSvcMainOpenProject->setEnabled(condition);
    ui->actionSvcMainNewProject->setEnabled(condition);
    ui->actionSlicerSettings->setEnabled(condition);
    ui->actiondeviceSettings->setEnabled(condition);
    saveEnabled = condition;
}

void SVCMainWindow::InsertFilePaths(std::wstring _str) const
{
    ui->SvcPlayListWidget->addItem(to_byte_string(_str.substr(_str.rfind(PathSeparator) + 1)).c_str());
}

void SVCMainWindow::ErrorMsgBoxEvent(std::string _str)
{
    QMessageBox::warning(this, "ERROR", _str.c_str(), QMessageBox::Ok);
}

//********************ActionSlotFn********************//
void SVCMainWindow::on_actionSvcMainNewProject_triggered()
{
    //if(!_model)
    //{
    //    QMessageBox::warning(this, tr("SvcPleaseLoadModelTitle"), tr("SvcPleaseLoadModelText"), QMessageBox::Ok);
    //    return;
    //}
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("SvcOverwriteTitle"));
    msgBox.setText(tr("SvcOverwriteText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("SvcOverwriteYes"));
    msgBox.button(QMessageBox::No)->setText(tr("SvcOverwriteNo"));
    if (!_Projects.empty() && msgBox.exec() == QMessageBox::No)
        return;
    _Projects.clear();
    try
    {
        const auto file_paths = QFileDialog::getOpenFileNames(this, tr("SvcCreateProjectWithAudio"), to_byte_string(GetCurrentFolder()).c_str(), "Audio files (*.wav *.ogg *.flac *.mp3);");
        if (file_paths.empty())
            return;
        for(const auto& file_path : file_paths)
        {
            MoeVSProject::Params _inferData;
            const auto AudioData = AudioPreprocess().codec(file_path.toStdWString(), 48000);
            const auto SliceInfo = SliceAudio(AudioData, 48000, _SlicerConfigs.threshold, _SlicerConfigs.minLen, _SlicerConfigs.frame_len, _SlicerConfigs.frame_shift);
            _inferData.symbolb = SliceInfo.second;
            _inferData.OrgAudio.reserve(SliceInfo.second.size());
            _inferData.OrgLen.reserve(SliceInfo.second.size());
            _inferData.F0.reserve(SliceInfo.second.size());
            _inferData.Volume.reserve(SliceInfo.second.size());

            for (size_t i = 1; i < SliceInfo.first.size(); ++i)
            {
                if (SliceInfo.second[i - 1])
                {
                    _inferData.OrgAudio.emplace_back(AudioData.data() + SliceInfo.first[i - 1], AudioData.data() + SliceInfo.first[i]);
                    auto OrgAudioDouble = InferClass::SVC::InterpResample(_inferData.OrgAudio[i - 1], 48000, 48000, 32768.0);
                    auto F0Extractor = F0PreProcess(48000, 320);
                    auto SrcF0 = F0Extractor.GetOrgF0(OrgAudioDouble.data(), int64_t(OrgAudioDouble.size()), int64_t(OrgAudioDouble.size() / 320) + 1, 0);
                    SrcF0 = mean_filter(SrcF0, ui->SvcParamsMFLenSpinBox->value());
                    _inferData.F0.emplace_back(std::move(SrcF0));
                    _inferData.Volume.emplace_back(mean_filter(InferClass::SVC::ExtractVolume(_inferData.OrgAudio[i - 1], 320), ui->SvcParamsMFLenSpinBox->value()));
                }
                else
                {
                    _inferData.OrgAudio.emplace_back();
                    _inferData.F0.emplace_back();
                    _inferData.Volume.emplace_back();
                }
                _inferData.Speaker.emplace_back();
                _inferData.OrgLen.emplace_back(SliceInfo.first[i] - SliceInfo.first[i - 1]);
            }
            _inferData.paths = file_path.toStdWString();
            _inferData.paths = _inferData.paths.substr(_inferData.paths.rfind(PathSeparator) + 1);
            _Projects.emplace_back(std::move(_inferData));
        }
    }
    catch(std::exception& e)
    {
        QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
    }
    ui->SvcProjectListWidget->clear();
    for (const auto& i : _Projects)
        ui->SvcProjectListWidget->addItem(to_byte_string(i.paths.substr(i.paths.rfind(PathSeparator) + 1)).c_str());
    CurProjectPath.clear();
    _curAudio = nullptr;
    cur_slice_index = -1;
}

void SVCMainWindow::on_actionSvcMainOpenProject_triggered()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("SvcOverwriteTitle"));
    msgBox.setText(tr("SvcOverwriteText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("SvcOverwriteYes"));
    msgBox.button(QMessageBox::No)->setText(tr("SvcOverwriteNo"));
    if (!_Projects.empty() && msgBox.exec() == QMessageBox::No)
        return;
    _Projects.clear();
    const auto file_path = QFileDialog::getOpenFileName(this, tr("SvcOpenProjectFile"), to_byte_string(GetCurrentFolder()).c_str(), "MoeVS Project File (*.mvsproj);");
    if (file_path.isEmpty())
        return;
    try
    {
        _Projects = MoeVSProject::MoeVSProject(file_path.toStdWString()).GetParamsMove();
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
    }
    ui->SvcProjectListWidget->clear();
    for (const auto& i : _Projects)
        ui->SvcProjectListWidget->addItem(to_byte_string(i.paths.substr(i.paths.rfind(PathSeparator) + 1)).c_str());
    CurProjectPath.clear();
    _curAudio = nullptr;
    cur_slice_index = -1;
}

void SVCMainWindow::on_actionSvcMainAppendProject_triggered()
{
    const auto file_path = QFileDialog::getOpenFileName(this, tr("SvcOpenProjectFile"), to_byte_string(GetCurrentFolder()).c_str(), "MoeVS Project File (*.mvsproj);");
    if (file_path.isEmpty())
        return;
    try
    {
    	auto T_Projects = MoeVSProject::MoeVSProject(file_path.toStdWString()).GetParamsMove();
        for (auto& i : T_Projects)
        {
	        ui->SvcProjectListWidget->addItem(to_byte_string(i.paths.substr(i.paths.rfind(PathSeparator) + 1)).c_str());
            _Projects.emplace_back(std::move(i));
        }
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
    }
}

void SVCMainWindow::on_actionSvcMainAddAudio_triggered()
{
    std::vector<std::wstring> _paths;
    try
    {
        const auto file_paths = QFileDialog::getOpenFileNames(this, tr("SvcCreateProjectWithAudio"), to_byte_string(GetCurrentFolder()).c_str(), "Audio files (*.wav *.ogg *.flac *.mp3);");
        if (file_paths.empty())
            return;
        for (const auto& file_path : file_paths)
        {
            MoeVSProject::Params _inferData;
            const auto AudioData = AudioPreprocess().codec(file_path.toStdWString(), 48000);
            const auto SliceInfo = SliceAudio(AudioData, 48000, _SlicerConfigs.threshold, _SlicerConfigs.minLen, _SlicerConfigs.frame_len, _SlicerConfigs.frame_shift);
            _inferData.symbolb = SliceInfo.second;
            _inferData.OrgAudio.reserve(SliceInfo.second.size());
            _inferData.OrgLen.reserve(SliceInfo.second.size());
            _inferData.F0.reserve(SliceInfo.second.size());
            _inferData.Volume.reserve(SliceInfo.second.size());

            for (size_t i = 1; i < SliceInfo.first.size(); ++i)
            {
                if (SliceInfo.second[i - 1])
                {
                    _inferData.OrgAudio.emplace_back(AudioData.data() + SliceInfo.first[i - 1], AudioData.data() + SliceInfo.first[i]);
                    auto OrgAudioDouble = InferClass::SVC::InterpResample(_inferData.OrgAudio[i - 1], 48000, 48000, 32768.0);
                    auto F0Extractor = F0PreProcess(48000, 320);
                    auto SrcF0 = F0Extractor.GetOrgF0(OrgAudioDouble.data(), int64_t(OrgAudioDouble.size()), int64_t(OrgAudioDouble.size() / 320) + 1, 0);
                    SrcF0 = mean_filter(SrcF0, ui->SvcParamsMFLenSpinBox->value());
                    _inferData.F0.emplace_back(std::move(SrcF0));
                    _inferData.Volume.emplace_back(mean_filter(InferClass::SVC::ExtractVolume(_inferData.OrgAudio[i - 1], 320), ui->SvcParamsMFLenSpinBox->value()));
                }
                else
                {
                    _inferData.OrgAudio.emplace_back();
                    _inferData.F0.emplace_back();
                    _inferData.Volume.emplace_back();
                }
                _inferData.Speaker.emplace_back();
                _inferData.OrgLen.emplace_back(SliceInfo.first[i] - SliceInfo.first[i - 1]);
            }
            _inferData.paths = file_path.toStdWString();
            _inferData.paths = _inferData.paths.substr(_inferData.paths.rfind(PathSeparator) + 1);
            _paths.emplace_back(_inferData.paths);
            _Projects.emplace_back(std::move(_inferData));
        }
    }
    catch (std::exception& e)
    {
        QMessageBox::warning(this, "ERROR", e.what(), QMessageBox::Ok);
    }
    for (const auto& i : _paths)
        ui->SvcProjectListWidget->addItem(to_byte_string(i.substr(i.rfind(PathSeparator) + 1)).c_str());
}

void SVCMainWindow::on_actionSvcMainSaveProject_triggered()
{
    if (_Projects.empty())
        return;
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("SvcOverwriteFileTitle"));
    msgBox.setText(tr("SvcOverwriteFileText"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(tr("SvcOverwriteFileYes"));
    msgBox.button(QMessageBox::No)->setText(tr("SvcOverwriteFileNo"));
    if (!CurProjectPath.empty() && msgBox.exec() == QMessageBox::No)
        return;
    if (CurProjectPath.empty())
        CurProjectPath = QFileDialog::getSaveFileName(this, tr("SvcSaveProjectFile"), to_byte_string(GetCurrentFolder()).c_str(), "MoeVS Project File (*.mvsproj);").toStdWString();
    if (CurProjectPath.empty())
        return;
    MoeVSProject::MoeVSProject(this->_Projects).Write(CurProjectPath);
}

void SVCMainWindow::on_actionSvcMainSaveProjectAs_triggered()
{
    if (_Projects.empty())
        return;
    const auto ProjectPath_ = QFileDialog::getSaveFileName(this, tr("SvcSaveProjectFile"), to_byte_string(GetCurrentFolder()).c_str(), "MoeVS Project File (*.mvsproj);").toStdWString();
    if (ProjectPath_.empty())
        return;
    MoeVSProject::MoeVSProject(this->_Projects).Write(ProjectPath_);
    CurProjectPath = ProjectPath_;
}

void SVCMainWindow::on_actionSvcMainExit_triggered()
{
	close();
}

void SVCMainWindow::on_actionSvcMainRedo_triggered()
{
	
}

void SVCMainWindow::on_actionSvcMainWithdraw_triggered()
{
	
}

void SVCMainWindow::on_actionSlicerSettings_triggered()
{
    _SettingSlicer_window.show();
}

void SVCMainWindow::on_actiondeviceSettings_triggered()
{
    QMessageBox::information(this, "TODO", "Comming Soon", QMessageBox::Ok);
}

void SVCMainWindow::on_actionSvcMainAboutPage_triggered()
{
    QMessageBox::information(this, "TODO", "Comming Soon", QMessageBox::Ok);
}

void SVCMainWindow::on_actionSvcMainHelpPage_triggered()
{
    QMessageBox::information(this, "TODO", "Comming Soon", QMessageBox::Ok);
}

void SVCMainWindow::on_actionSvcMainPluginsManager_triggered()
{
    QMessageBox::information(this, "TODO", "Comming Soon", QMessageBox::Ok);
}

//***************ValueChangedSlotFn*******************//

void SVCMainWindow::on_SvcPlayerPlayPosHorizontalSlider_valueChanged(int value) const
{
    if (!duration_media)
        return;
    if (value == duration_media) {
        ui->SvcPlayerPlayPosHorizontalSlider->setValue(0);
        media_player->stop();
        media_player->setPosition(0);
        return;
    }
    media_player->setPosition(value);
}

void SVCMainWindow::on_SvcPlayerVolumeSlider_valueChanged(int value) const
{
    media_audio_output->setVolume(float(value) / 100);
    if (!value)
    {
        //TODO
    }
}

void SVCMainWindow::on_SvcPlayListWidget_itemDoubleClicked(QListWidgetItem* item) const
{
    if (item)
    {
        const auto idx = ui->SvcPlayListWidget->row(item);
        setAudio(_audioFolder[idx]);
    }
}