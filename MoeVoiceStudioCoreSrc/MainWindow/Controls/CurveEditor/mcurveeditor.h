#ifndef MCURVEEDITOR_H
#define MCURVEEDITOR_H
#include "Project.hpp"
#include "editorpage.h"
#include "ui_mcurveeditor.h"
#include <QMediaPlayer>
#include <QAudioOutput>

namespace Ui {
class MCurveEditor;
}

class MCurveEditor : public QWidget
{
    Q_OBJECT

public:
    struct RtnData
    {
    	std::vector<float> F0;
    	std::vector<float> Volume;
    	std::vector<float> Speaker;
    };
    explicit MCurveEditor(QWidget *parent = nullptr);
    ~MCurveEditor() override;
    [[nodiscard]] double getSpkData(int gidx, int idx) const;
    char SaveToNpyData(const std::vector<float>& _data, const std::vector<int64_t>& _shape);
    void setOpenAudioEnabled(bool condition) const
    {
        ui->SvcEditorOpenAudioButton->setEnabled(condition);
        ui->SvcEditorOpenF0NpyButton->setEnabled(condition);
    }
    /***********InitFn************/
    void initControl();
    void setData(const std::vector<float>& F0,
        const std::vector<float>& Volume,
        const std::vector<float>& Speaker,
        const std::vector<int16_t>& _audio_Data,
        const std::vector<std::string>& _speaker,
        const int samplingRate);
    [[nodiscard]] RtnData getData() const;
    [[nodiscard]] std::vector<float> getF0Data() const;
    [[nodiscard]] std::vector<float> getVolumeData() const;
    [[nodiscard]] std::vector<float> getSpeakerData() const;
    void setPlayerData(const int samplingRate);
    void clearPlots()
    {
        initControl();
    }
    /***********Update************/


    /***********Event*************/
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
public slots:
    void on_SvcEditorHorizontalScrollBar_valueChanged(int value);
    void on_VolumeScrollBar_valueChanged(int value);
    void on_SpkScrollBar_valueChanged(int value);
    void on_F0ScrollBar_valueChanged(int value);
    void on_SpkMixWidget_mouseDoubleClick(QMouseEvent* event) const;
    void on_VolumeWidget_mouseDoubleClick(QMouseEvent* event) const;
    void on_F0Widget_mouseDoubleClick(QMouseEvent* event) const;
    void X_Widget_rangeChanged(QCPRange range) const;
    void Y_SpkMixWidget_rangeChanged(QCPRange range) const;
    void Y_VolumeWidget_rangeChanged(QCPRange range) const;
    void Y_F0Widget_rangeChanged(QCPRange range) const;
    void on_SpeakerComboBox_currentIndexChanged(int index) const;
    void on_SaveF0ToNpy_clicked();
    void on_SaveVolumeToNpy_clicked();
    void on_SaveSpeakerMixToNpy_clicked();
    void on_SvcEditorOpenAudioButton_clicked();
    void on_SvcEditorOpenF0NpyButton_clicked();
    void getDurationChanged(qint64 time);
    void playbackPosChanged(qint64 time) const;
private:
    void setPlayProgress(double pos_) const
    {
        ui->F0Widget->setProgress(pos_);
        ui->SpkMixWidget->setProgress(pos_);
        ui->VolumeWidget->setProgress(pos_);
    }
    void setProgressVisable(bool _cond) const
    {
        ui->F0Widget->ProgressEnabled(_cond);
        ui->SpkMixWidget->ProgressEnabled(_cond);
        ui->VolumeWidget->ProgressEnabled(_cond);
    }
    double CenterC = 261.626;
    double CenterCPitch = 4 * 12 + 1;
    std::vector<int16_t> _audioData;
    Ui::MCurveEditor *ui;
    int n_frames = 0;
    bool volume_enabled = false;
    bool spk_enabled = false;
    bool f0_enabled = false;
    QMediaPlayer* media_player = nullptr;
    QAudioOutput* media_audio_output = nullptr;
    qint64 _Duration = 0;
    qint64 _CurPos = 0;
    std::function<void(double)> _click_callback = [&](double time)
    {
        setPlayProgress(time);
        media_player->setPosition(qint64(time / double(n_frames) * double(_Duration)));
    };
};

#endif // MCURVEEDITOR_H
