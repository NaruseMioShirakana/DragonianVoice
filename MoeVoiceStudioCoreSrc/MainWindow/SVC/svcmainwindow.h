#ifndef SVCMAINWINDOW_H
#define SVCMAINWINDOW_H

#include <ModelBase.hpp>
#include <QtMultimedia/QAudioDevice>
#include "MessageSender.h"
#include "ui_svcmainwindow.h"
#include "svcinferslicersetting.h"
#include "moevssetting.h"

namespace Ui {
class SVCMainWindow;
}

class SVCMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MoeVSSetting _setting_widget;
    struct SliceConfig
    {
        double threshold = 40;
    	unsigned long minLen = 3;
    	unsigned short frame_len = 1024;
    	unsigned short frame_shift = 256;
    };
    explicit SVCMainWindow(const std::function<void(QDialog*)>& _callback, QWidget *parent = nullptr);
    ~SVCMainWindow() override;
    void unloadModel();
    void initPlayer();
    void initSetup();
    void reloadModels();
    void modelsClear();
    void loadModel(size_t idx);
    void SetInferenceEnabled(bool condition) const;
    void setSoundModelEnabled(bool condition) const;
    void SetModelSelectEnabled(bool condition) const;
    void setAudio(const std::wstring& path) const
	{
        media_player->setSource(QUrl::fromLocalFile(to_byte_string(path).c_str()));
        ui->SvcPlayerWaveWidget->setWavFile(path);
        ui->SvcPlayerWaveWidget->update();
    }

    //*******************EventsFn*************************//
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

public slots:
    //*******************AudioSlotFn**********************//
    void getDurationChanged(qint64 time);
    void playbackPosChanged(qint64 time) const;
    //******************ButtonSlotFn**********************//
    void on_vcLoadModelButton_clicked();
    void on_vcReLoadModelButton_clicked();
    void on_SvcProjectDelPushButton_clicked();
    void on_SvcProjectInferCurPushButton_clicked();
    void on_SvcMoeSSInfer_clicked();
    void on_SvcPlayerMuteButton_clicked() const;
    void on_SvcPlayerPlayButton_clicked() const;
    void on_SvcSaveSegment_clicked() const;
    void on_SvcInferCurSlice_clicked();
#ifdef WIN32
    static void on_SvcOpenModelFolderButton_clicked();
#endif
    //***************ValueChangedSlotFn*******************//
    void on_SvcPlayerVolumeSlider_valueChanged(int value) const;
    void on_SvcPlayerPlayPosHorizontalSlider_valueChanged(int value) const;
    void on_SvcPlayListWidget_itemDoubleClicked(QListWidgetItem* item) const;

    //*********************SigSlotFn**********************//
    void InferenceProcessChanged(size_t cur, size_t all) const;
    void InferenceStatChanged(bool condition);
    void InsertFilePaths(std::wstring _str) const;
    void ErrorMsgBoxEvent(std::string _str);

    //*********************clickedFn**********************//
    void on_SvcProjectListWidget_itemDoubleClicked(QListWidgetItem* item);
    void on_SvcSegmentListWidget_itemDoubleClicked(QListWidgetItem* item);

    //********************ActionSlotFn********************//
    void on_actionSvcMainNewProject_triggered();
    void on_actionSvcMainOpenProject_triggered();
    void on_actionSvcMainAppendProject_triggered();
    void on_actionSvcMainAddAudio_triggered();
    void on_actionSvcMainSaveProject_triggered();
    void on_actionSvcMainSaveProjectAs_triggered();
    void on_actionSvcMainExit_triggered();

    void on_actionSvcMainRedo_triggered();
    void on_actionSvcMainWithdraw_triggered();

    void on_actionSlicerSettings_triggered();
    void on_actiondeviceSettings_triggered();

    void on_actionSvcMainPluginsManager_triggered();

    void on_actionSvcMainAboutPage_triggered();
    void on_actionSvcMainHelpPage_triggered();
private:
    Ui::SVCMainWindow* ui;
    SliceConfig _SlicerConfigs;
    int samplingRate = 22050;
    int duration_media = 0;
    int n_speakers = 0;
    std::vector<std::wstring> _audioFolder;

    std::vector<MJson> _models;
    InferClass::SVC* _model = nullptr;
    std::wstring CurProjectPath;

    QMediaPlayer* media_player = nullptr;
    QAudioOutput* media_audio_output = nullptr;
    size_t cur_model_index = 0;
    
    MessageSender _InferMsgSender;
    bool saveEnabled = true;

    std::vector<MoeVSProject::Params> _Projects;
    MoeVSProject::Params* _curAudio = nullptr;
    size_t cur_slice_index = -1;
    std::vector<std::string> _speakers;
    SvcInferSlicerSetting _SettingSlicer_window{ _SlicerSettingCallBack };

    const std::function<void(QDialog*)>& exit_callback;

public:
	std::function<void(double, long, long, long)> _SlicerSettingCallBack = [&](double threshold, long minLen, long frame_len, long frame_shift)
    {
        _SlicerConfigs.threshold = threshold;
        _SlicerConfigs.frame_len = short(frame_len);
        _SlicerConfigs.frame_shift = short(frame_shift);
        _SlicerConfigs.minLen = minLen;
    };
    InferClass::OnnxModule::callback BarCallback = [&](size_t _cur, size_t _max) -> void
    {
        _InferMsgSender.InferProcess(_cur, _max);
    };
    InferClass::OnnxModule::callback_params TTSParamCallback = [&]()
    {
        InferClass::InferConfigs _conf;
        _conf.chara = ui->SvcCharacterSelector->currentIndex();
        if (_conf.chara < 0)
            _conf.chara = 0;
        _conf.noise_scale = (float)ui->SvcParamsNoiseScaleSpinBox->value();
        _conf.noise_scale_w = (float)ui->SvcParamsNoiseScaleDDSPSpinBox->value();
        _conf.index_rate = float(ui->SvcParamsIndexRateSpinBox->value());
        if (_conf.index_rate < 0.001f)
            _conf.index = false;
        else
            _conf.index = true;
        _conf.seed = ui->SvcParamsSeedsSpinBox->value();
        _conf.keys = ui->SvcParamsUpKeySpinBox->value();
        _conf.kmeans_rate = float(ui->SvcParamsKmeansRateSpinBox->value());
        _conf.pndm = ui->SvcParamsPndmSpinBox->value();
        _conf.step = ui->SvcParamsStepSpinBox->value();
        _conf.cp = false;
        _conf.threshold = long(_SlicerConfigs.threshold);
        _conf.frame_len = _SlicerConfigs.frame_len;
        _conf.frame_shift = _SlicerConfigs.frame_shift;
        _conf.minLen = _SlicerConfigs.minLen;
        _conf.filter_window_len = ui->SvcParamsMFLenSpinBox->value();
        return _conf;
    };

    const wchar_t PathSeparator = L'/';
};

#endif // SVCMAINWINDOW_H
