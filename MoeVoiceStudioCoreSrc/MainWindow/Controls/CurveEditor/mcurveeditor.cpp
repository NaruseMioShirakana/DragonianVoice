#include "mcurveeditor.h"
#include <qaudioformat.h>
#include <QAudioDevice>
#include "ui_mcurveeditor.h"
#include <QWheelEvent>
#include <QScrollBar>
#include "ModelBase.hpp"

MCurveEditor::MCurveEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MCurveEditor)
{
    ui->setupUi(this);
	initControl();
	ui->F0Widget->xAxis->setVisible(false);
	ui->SpkMixWidget->xAxis->setVisible(false);
	ui->VolumeWidget->setMaxValue(1.0);
	ui->SpkMixWidget->setMaxValue(1.0);
	ui->F0Widget->setMaxValue(128.0);
	ui->VolumeWidget->setCallback(_click_callback);
	ui->SpkMixWidget->setCallback(_click_callback);
	ui->F0Widget->setCallback(_click_callback);
	connect(ui->VolumeWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(X_Widget_rangeChanged(QCPRange)));
	connect(ui->SpkMixWidget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(X_Widget_rangeChanged(QCPRange)));
	connect(ui->F0Widget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(X_Widget_rangeChanged(QCPRange)));
	connect(ui->F0Widget->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(Y_F0Widget_rangeChanged(QCPRange)));
	connect(ui->VolumeWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(Y_VolumeWidget_rangeChanged(QCPRange)));
	connect(ui->SpkMixWidget->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(Y_SpkMixWidget_rangeChanged(QCPRange)));
	const QSharedPointer<MPitchLabel> PitchTicker(new MPitchLabel());
	ui->F0Widget->yAxis->setTicker(PitchTicker);
	media_player = new QMediaPlayer(this);
	media_audio_output = new QAudioOutput(this);
	media_player->setAudioOutput(media_audio_output);
	media_audio_output->setDevice(QAudioDevice());
	media_audio_output->setVolume(80);
	connect(media_player, SIGNAL(durationChanged(qint64)), this, SLOT(getDurationChanged(qint64)));
	connect(media_player, SIGNAL(positionChanged(qint64)), this, SLOT(playbackPosChanged(qint64)));
}

void MCurveEditor::initControl()
{
	ui->F0ScrollBar->setRange(0, 64);
	ui->VolumeScrollBar->setRange(0, 500);
	ui->SpkScrollBar->setRange(0, 500);
	ui->SvcEditorHorizontalScrollBar->setRange(0, 1);

	ui->F0ScrollBar->setPageStep(64);       //    1 : 1
	ui->SpkScrollBar->setPageStep(500);     // 1000 : 1
	ui->VolumeScrollBar->setPageStep(500);  // 1000 : 1
	ui->SvcEditorHorizontalScrollBar->setPageStep(100);

	ui->VolumeWidget->clearGraphs();
	ui->F0Widget->clearGraphs();
	ui->SpkMixWidget->clearGraphs();

	ui->SpeakerComboBox->clear();
	_CurPos = 0;
	_Duration = 0;
	n_frames = 0;
	if(media_player)
		media_player->setSource(QUrl());

	ui->F0Widget->clearBackup();
	ui->VolumeWidget->clearBackup();
	ui->SpkMixWidget->clearBackup();

	f0_enabled = false;
	spk_enabled = false;
	volume_enabled = false;

	ui->VolumeWidget->yAxis->setRange(0, 0.5);
	ui->SpkMixWidget->yAxis->setRange(0, 0.5);
	ui->F0Widget->yAxis->setRange(0, 64);

	ui->SpkMixWidget->xAxis->setRange(0, 100);
	ui->VolumeWidget->xAxis->setRange(0, 100);
	ui->F0Widget->xAxis->setRange(0, 100);

	ui->SpkMixWidget->enableChara();
}

MCurveEditor::~MCurveEditor()
{
    delete ui;
}

void MCurveEditor::setData(
	const std::vector<float>& F0,
	const std::vector<float>& Volume,
	const std::vector<float>& Speaker,
	const std::vector<int16_t>& _audio_Data,
	const std::vector<std::string>& _speaker,
	const int samplingRate)
{
	const int n_speaker = int(_speaker.size());
	n_frames = int(F0.size());
	if (!n_frames)
		n_frames = int(Volume.size());
	if (!n_frames)
		n_frames = int(Speaker.size()) / n_speaker;
	ui->SvcEditorHorizontalScrollBar->setRange(0, n_frames - int(ui->F0Widget->xAxis->range().size()));
	ui->SpkMixWidget->clearGraphs();
	ui->VolumeWidget->clearGraphs();
	ui->F0Widget->clearGraphs();
	ui->SpeakerComboBox->clear();
	ui->F0Widget->clearBackup();
	ui->VolumeWidget->clearBackup();
	ui->SpkMixWidget->clearBackup();
	if (!F0.empty())
	{
		ui->F0Widget->addGraph();
		QVector<double> F0Data, XaxisData;
		F0Data.reserve(n_frames);
		XaxisData.reserve(n_frames);
		for (size_t i = 0; i < F0.size(); ++i)
		{
			F0Data.emplace_back((12 * log(double(F0[i])/CenterC)/log(2)) + CenterCPitch);
			XaxisData.emplace_back(double(i));
		}
		ui->F0Widget->graph(0)->setData(XaxisData, F0Data);
		f0_enabled = true;
		ui->F0Widget->replot();
	}
	else
		f0_enabled = false;
	if (!Volume.empty())
	{
		ui->VolumeWidget->addGraph();
		QVector<double> VolumeData, XaxisData;
		VolumeData.reserve(n_frames);
		XaxisData.reserve(n_frames);
		for (size_t i = 0; i < Volume.size(); ++i)
		{
			VolumeData.emplace_back(double(Volume[i]));
			XaxisData.emplace_back(double(i));
		}
		if(VolumeData.size() < n_frames)
		{
			XaxisData.emplace_back(VolumeData.size());
			VolumeData.emplace_back(0.0);
		}
		ui->VolumeWidget->graph(0)->setData(XaxisData, VolumeData);
		volume_enabled = true;
		ui->VolumeWidget->replot();
	}
	else
		volume_enabled = false;
	if (!Speaker.empty())
	{
		size_t offset_g = 0;
		if (Speaker.size() == size_t(n_frames * n_speaker + 1))
			offset_g = 1;
		QVector<double> XaxisData;
		XaxisData.reserve(n_frames);
		QVector<QVector<double>> SpeakerData;
		for (int i = 0; i < n_speaker; ++i)
		{
			ui->SpeakerComboBox->addItem(_speaker[i].c_str());
			SpeakerData.emplace_back();
			SpeakerData[i].reserve(n_frames);
			ui->SpkMixWidget->addGraph();
			ui->SpkMixWidget->graph(i)->setName(_speaker[i].c_str());
		}
		for (int i = 0; i < n_frames; ++i)
		{
			XaxisData.emplace_back(double(i));
			for (int j = 0; j < n_speaker; ++j)
				SpeakerData[j].emplace_back(Speaker[i * n_speaker + j + offset_g]);
		}
		for (int i = 0; i < n_speaker; ++i)
		{
			ui->SpkMixWidget->graph(i)->setData(XaxisData, SpeakerData[i]);
			ui->SpkMixWidget->graph(i)->setPen(QPen(QColor(rand() % 256, rand() % 256, rand() % 256)));
		}
		spk_enabled = true;
		ui->SpkMixWidget->replot();
	}
	else
		spk_enabled = false;
	if (!_audio_Data.empty())
		_audioData = _audio_Data;
	if(!_audioData.empty())
		setPlayerData(samplingRate);
}

void MCurveEditor::setPlayerData(const int samplingRate)
{
	media_player->setSource(QUrl());
	const std::string _AudioPath = to_byte_string(GetCurrentFolder() + L"/etmp.wav");
	QFile _AudioFile(_AudioPath.c_str());
	if (!_AudioFile.open(QIODevice::WriteOnly))
	{
		_Duration = 0;
		return;
	}
	Wav::WAV_HEADER wav_header;
	wav_header.SamplesPerSec = samplingRate;
	wav_header.bytesPerSec = samplingRate * 2;
	wav_header.Subchunk2Size = _audioData.size() * 2;
	wav_header.ChunkSize += wav_header.Subchunk2Size;
	_AudioFile.write((const char*)(&wav_header), sizeof(Wav::WAV_HEADER));
	_AudioFile.write((const char*)(_audioData.data()), qint64(_audioData.size() * 2));
	_AudioFile.close();
	media_player->setSource(QUrl::fromLocalFile(_AudioPath.c_str()));
	//media_player->play();
	//media_player->setSource;
}

void MCurveEditor::keyPressEvent(QKeyEvent* event)
{
	if(event->key()==Qt::Key_Space && _Duration)
	{
		switch (media_player->playbackState())
		{
		case QMediaPlayer::PlayingState:
		{
			setProgressVisable(false);
			media_player->pause();
			break;
		}
		case QMediaPlayer::PausedState:
		{
			setProgressVisable(true);
			media_player->play();
			break;
		}
		case QMediaPlayer::StoppedState:
		{
			setProgressVisable(true);
			media_player->play();
			break;
		}
		}
		return;
	}
	QWidget::keyPressEvent(event);
}

double MCurveEditor::getSpkData(int gidx, int idx) const
{
	return (ui->SpkMixWidget->graph(gidx)->data().data()->begin() + idx)->value;
}

MCurveEditor::RtnData MCurveEditor::getData() const
{
	RtnData ret;
	const int n_speaker = ui->SpkMixWidget->graphCount();
	if(f0_enabled)
	{
		const auto F0Data = ui->F0Widget->graph(0)->data().data();
		for (const auto& i : *F0Data)
			ret.F0.emplace_back((pow(2, (i.value - CenterCPitch) / 12)) * CenterC);
	}
	if(volume_enabled)
	{
		const auto VolData = ui->VolumeWidget->graph(0)->data().data();
		for (const auto& i : *VolData)
			ret.Volume.emplace_back(i.value);
	}
	if(spk_enabled)
	{
		ret.Speaker.emplace_back(n_speaker);
		for (int i = 0; i < n_frames; ++i)
			for (int j = 0; j < n_speaker; ++j)
				ret.Speaker.emplace_back(getSpkData(j, i));
	}
	return ret;
}

std::vector<float> MCurveEditor::getF0Data() const
{
	std::vector<float> F0;
	if (f0_enabled)
	{
		const auto F0Data = ui->F0Widget->graph(0)->data().data();
		for (const auto& i : *F0Data)
			F0.emplace_back((pow(2, (i.value - CenterCPitch) / 12)) * CenterC);
	}
	return F0;
}

std::vector<float> MCurveEditor::getSpeakerData() const
{
	std::vector<float> Speaker;
	const int n_speaker = ui->SpkMixWidget->graphCount();
	if (spk_enabled)
	{
		for (int i = 0; i < n_frames; ++i)
			for (int j = 0; j < n_speaker; ++j)
				Speaker.emplace_back(getSpkData(j, i));
	}
	return Speaker;
}

std::vector<float> MCurveEditor::getVolumeData() const
{
	std::vector<float> Volume;
	if (volume_enabled)
	{
		const auto VolData = ui->VolumeWidget->graph(0)->data().data();
		for (const auto& i : *VolData)
			Volume.emplace_back(i.value);
	}
	return Volume;
}

void MCurveEditor::wheelEvent(QWheelEvent* event)
{
	const auto y_move = event->angleDelta().y();
	if(event->modifiers() == Qt::ShiftModifier)
	{
		if (y_move < 0)
			ui->SvcEditorHorizontalScrollBar->setValue(ui->SvcEditorHorizontalScrollBar->value() + 20);
		else if (y_move > 0)
			ui->SvcEditorHorizontalScrollBar->setValue(ui->SvcEditorHorizontalScrollBar->value() - 20);
	}
}

void MCurveEditor::on_F0ScrollBar_valueChanged(int value)
{
	const auto _size = ui->F0Widget->yAxis->range().size();
	ui->F0Widget->yAxis->setRange(value, value + _size);
	ui->F0Widget->replot();
	update();
}

void MCurveEditor::on_SpkScrollBar_valueChanged(int value)
{
	const auto _size = ui->SpkMixWidget->yAxis->range().size();
	ui->SpkMixWidget->yAxis->setRange(value / 1000.0, value / 1000.0 + _size);
	ui->SpkMixWidget->replot();
	update();
}

void MCurveEditor::on_VolumeScrollBar_valueChanged(int value)
{
	const auto _size = ui->VolumeWidget->yAxis->range().size();
	ui->VolumeWidget->yAxis->setRange(value / 1000.0, value / 1000.0 + _size);
	ui->VolumeWidget->replot();
	update();
}

void MCurveEditor::on_SvcEditorHorizontalScrollBar_valueChanged(int value)
{
	const auto _size = ui->SpkMixWidget->xAxis->range().size();
	ui->SpkMixWidget->xAxis->setRange(value, value + _size);
	ui->VolumeWidget->xAxis->setRange(value, value + _size);
	ui->F0Widget->xAxis->setRange(value, value + _size);
	ui->SpkMixWidget->replot();
	ui->VolumeWidget->replot();
	ui->F0Widget->replot();
	update();
}

void MCurveEditor::on_F0Widget_mouseDoubleClick(QMouseEvent* event) const
{
	if(event->button() == Qt::LeftButton)
	{
		if (event->modifiers() == Qt::NoModifier)
		{
			ui->F0Widget->setVisible(true);
			ui->SpkMixWidget->setVisible(false);
			ui->VolumeWidget->setVisible(false);
			ui->F0ScrollBar->setVisible(true);
			ui->VolumeScrollBar->setVisible(false);
			ui->SpkScrollBar->setVisible(false);

			ui->F0Widget->xAxis->setVisible(true);
		}
		else if (event->modifiers() == Qt::ControlModifier)
		{
			ui->F0Widget->setVisible(true);
			ui->SpkMixWidget->setVisible(true);
			ui->VolumeWidget->setVisible(true);
			ui->F0ScrollBar->setVisible(true);
			ui->VolumeScrollBar->setVisible(true);
			ui->SpkScrollBar->setVisible(true);
			ui->F0Widget->xAxis->setVisible(false);
		}
		ui->F0Widget->replot();
	}
}

void MCurveEditor::on_SpkMixWidget_mouseDoubleClick(QMouseEvent* event) const
{
	if (event->button() == Qt::LeftButton)
	{
		if(event->modifiers() == Qt::NoModifier)
		{
			ui->F0Widget->setVisible(false);
			ui->SpkMixWidget->setVisible(true);
			ui->VolumeWidget->setVisible(false);
			ui->F0ScrollBar->setVisible(false);
			ui->VolumeScrollBar->setVisible(false);
			ui->SpkMixWidget->setVisible(true);

			ui->SpkMixWidget->xAxis->setVisible(true);
		}
		else if (event->modifiers() == Qt::ControlModifier)
		{
			ui->F0Widget->setVisible(true);
			ui->SpkMixWidget->setVisible(true);
			ui->VolumeWidget->setVisible(true);
			ui->F0ScrollBar->setVisible(true);
			ui->VolumeScrollBar->setVisible(true);
			ui->SpkScrollBar->setVisible(true);
			ui->SpkMixWidget->xAxis->setVisible(false);
		}
		ui->SpkMixWidget->replot();
	}
}

void MCurveEditor::on_VolumeWidget_mouseDoubleClick(QMouseEvent* event) const
{
	if (event->button() == Qt::LeftButton)
	{
		if (event->modifiers() == Qt::NoModifier)
		{
			ui->F0Widget->setVisible(false);
			ui->SpkMixWidget->setVisible(false);
			ui->VolumeWidget->setVisible(true);
			ui->F0ScrollBar->setVisible(false);
			ui->VolumeScrollBar->setVisible(true);
			ui->SpkScrollBar->setVisible(false);
		}
		else if (event->modifiers() == Qt::ControlModifier)
		{
			ui->F0Widget->setVisible(true);
			ui->SpkMixWidget->setVisible(true);
			ui->VolumeWidget->setVisible(true);
			ui->F0ScrollBar->setVisible(true);
			ui->VolumeScrollBar->setVisible(true);
			ui->SpkScrollBar->setVisible(true);
		}
	}
}

void MCurveEditor::X_Widget_rangeChanged(QCPRange range) const
{
	const auto rsize = range.size();
	if(n_frames)
	{
		if(range.lower < 0)
		{
			range.lower = 0;
			if ((int)ceil(rsize) > n_frames)
				range.upper = (double)n_frames;
			else
				range.upper = rsize + range.lower;
		}
		else if(range.upper > n_frames)
		{
			range.upper = (double)n_frames;
			if ((int)ceil(rsize) > n_frames)
				range.lower = 0;
			else
				range.lower = range.upper - rsize;
		}
		if (rsize < 1.0)
		{
			range.lower = int(range.lower);
			range.upper = range.lower + 1.0;
		}
		ui->F0Widget->xAxis->setRange(range);
		ui->SpkMixWidget->xAxis->setRange(range);
		ui->VolumeWidget->xAxis->setRange(range);

		ui->SvcEditorHorizontalScrollBar->setMaximum(n_frames - int(range.size()));
		ui->SvcEditorHorizontalScrollBar->setValue((int)range.lower);
		ui->SvcEditorHorizontalScrollBar->setPageStep((int)ceil(range.size()));

		ui->SpkMixWidget->replot();
		ui->VolumeWidget->replot();
		ui->F0Widget->replot();
	}
}

void MCurveEditor::Y_F0Widget_rangeChanged(QCPRange range) const
{
	const auto rsize = range.size();
	if (n_frames)
	{
		if (range.lower < 0)
		{
			range.lower = 0;
			if ((int)ceil(rsize) > 128)
				range.upper = (double)128;
			else
				range.upper = rsize + range.lower;
		}
		else if (range.upper > 128)
		{
			range.upper = (double)128;
			if ((int)ceil(rsize) > 128)
				range.lower = 0;
			else
				range.lower = range.upper - rsize;
		}
		if (rsize < 1.0)
		{
			range.lower = int(range.lower);
			range.upper = range.lower + 1.0;
		}
		ui->F0ScrollBar->setMaximum(128 - int(range.size()));
		ui->F0ScrollBar->setValue((int)range.lower);
		ui->F0ScrollBar->setPageStep((int)ceil(range.size()));
		ui->F0Widget->yAxis->setRange(range);
		ui->F0Widget->replot();
	}
}

void MCurveEditor::Y_SpkMixWidget_rangeChanged(QCPRange range) const
{
	const auto rsize = range.size();
	if (n_frames)
	{
		if (range.lower < 0)
		{
			range.lower = 0;
			if ((int)ceil(rsize) > 1.0)
				range.upper = 1.0;
			else
				range.upper = rsize + range.lower;
		}
		else if (range.upper > 1.0)
		{
			range.upper = 1.0;
			if ((int)ceil(rsize) > 1.0)
				range.lower = 0;
			else
				range.lower = range.upper - rsize;
		}
		if (rsize < 0.001)
			range.upper = range.lower + 0.001;
		ui->SpkScrollBar->setMaximum(1000 - int(range.size() * 1000));
		ui->SpkScrollBar->setValue((int)(range.lower * 1000));
		ui->SpkScrollBar->setPageStep((int)ceil(range.size() * 1000));
		ui->SpkMixWidget->yAxis->setRange(range);
		ui->SpkMixWidget->replot();
	}
}

void MCurveEditor::Y_VolumeWidget_rangeChanged(QCPRange range) const
{
	const auto rsize = range.size();
	if (n_frames)
	{
		if (range.lower < 0)
		{
			range.lower = 0;
			if ((int)ceil(rsize) > 1.0)
				range.upper = 1.0;
			else
				range.upper = rsize + range.lower;
		}
		else if (range.upper > 1.0)
		{
			range.upper = 1.0;
			if ((int)ceil(rsize) > 1.0)
				range.lower = 0;
			else
				range.lower = range.upper - rsize;
		}
		if (rsize < 0.001)
			range.upper = range.lower + 0.001;
		ui->VolumeScrollBar->setMaximum(1000 - int(range.size() * 1000));
		ui->VolumeScrollBar->setValue((int)(range.lower * 1000));
		ui->VolumeScrollBar->setPageStep((int)ceil(range.size() * 1000));
		ui->VolumeWidget->yAxis->setRange(range);
		ui->VolumeWidget->replot();
	}
}

void MCurveEditor::on_SpeakerComboBox_currentIndexChanged(int index) const
{
	if(index< ui->SpkMixWidget->graphCount())
		ui->SpkMixWidget->changeGraph(index);
}

char MCurveEditor::SaveToNpyData(const std::vector<float>& _data, const std::vector<int64_t>& _shape)
{
	const auto file_path = QFileDialog::getSaveFileName(this, tr("SvcSaveNpyData"), to_byte_string(GetCurrentFolder()).c_str(), "Numpy files (*.npy);");
	if (file_path.isEmpty())
		return 1;
	FILE* outFile;
	_wfopen_s(&outFile, file_path.toStdWString().c_str(), L"wb");
	if (!outFile)
		return -1;
	std::vector _npyHeader{ char(147), 'N', 'U', 'M', 'P', 'Y', '\1', '\0', char(118), '\0'};
	std::string _npyHeaderDesc = "{'descr': '<f4', 'fortran_order': False, 'shape': ("
		+ std::to_string(_shape[0]) + ", "
		+ std::to_string(_shape[1]) + +"), }";
	_npyHeader.insert(_npyHeader.end(), _npyHeaderDesc.begin(), _npyHeaderDesc.end());
	while (_npyHeader.size() < 127)
		_npyHeader.emplace_back(' ');
	_npyHeader.emplace_back((char)10);
	fwrite(_npyHeader.data(), 1, 128, outFile);
	fwrite(_data.data(), 1, _data.size() * sizeof(float), outFile);
	fclose(outFile);
	return 0;
}

void MCurveEditor::on_SaveF0ToNpy_clicked()
{
	const auto data = getF0Data();
	if (SaveToNpyData(data, { int64_t(data.size()),1 }) == -1)
		QMessageBox::warning(this, tr("SvcNpySaveErrorTitle"), tr("SvcNpySaveErrorText"), QMessageBox::Ok);
}

void MCurveEditor::on_SaveSpeakerMixToNpy_clicked()
{
	const auto data = getSpeakerData();
	if (SaveToNpyData(data, { int64_t(data.size()),ui->SpkMixWidget->graphCount() }) == -1)
		QMessageBox::warning(this, tr("SvcNpySaveErrorTitle"), tr("SvcNpySaveErrorText"), QMessageBox::Ok);
}

void MCurveEditor::on_SaveVolumeToNpy_clicked()
{
	const auto data = getVolumeData();
	if (SaveToNpyData(data, { int64_t(data.size()),1 }) == -1)
		QMessageBox::warning(this, tr("SvcNpySaveErrorTitle"), tr("SvcNpySaveErrorText"), QMessageBox::Ok);
}

void MCurveEditor::on_SvcEditorOpenAudioButton_clicked()
{
	const auto file_path = QFileDialog::getOpenFileName(this, tr("SvcEditorOpenAudioFile"), to_byte_string(GetCurrentFolder()).c_str(), "Audio (*.wav *.flac *.mp3 *.ogg);");
	if (file_path.isEmpty())
		return;
	try
	{
		_audioData = AudioPreprocess().codec(file_path.toStdWString(), ui->SvcEditorSamplingRateBox->value());
	}
	catch (std::exception& e)
	{
		QMessageBox::warning(this, tr("OpenFileError"), e.what(), QMessageBox::Ok);
	}
	const auto Volume = InferClass::SVC::ExtractVolume(_audioData, ui->HopSizeSpinBox->value());
	auto F0Extractor = F0PreProcess(ui->SvcEditorSamplingRateBox->value(), short(ui->HopSizeSpinBox->value()));
	std::vector<double> DoubleAudio;
	DoubleAudio.reserve(_audioData.size());
	for (const auto i : _audioData)
		DoubleAudio.emplace_back(double(i) / 32768.);
	const auto F0 = F0Extractor.GetOrgF0(DoubleAudio.data(), 
		int64_t(DoubleAudio.size()), 
		int64_t(DoubleAudio.size()) / ui->HopSizeSpinBox->value() + 1,
		0);
	const auto n_spk = ui->NumSpeakerSpinBox->value();
	std::vector<float> spkMixd(n_spk, 0);
	std::vector<float> spkMixData;
	spkMixData.reserve(F0.size() * n_spk);
	std::vector<std::string> SpeakerNames;
	spkMixd[0] = 1.0;
	for (int i = 0; i < n_spk; ++i)
		SpeakerNames.emplace_back("Speaker" + std::to_string(i));
	for (size_t i = 0; i < F0.size(); ++i)
		spkMixData.insert(spkMixData.end(), spkMixd.begin(), spkMixd.end());
	setData(F0, Volume, spkMixData, std::vector<int16_t>(), SpeakerNames, ui->SvcEditorSamplingRateBox->value());
}

void MCurveEditor::getDurationChanged(qint64 time) {
	_Duration = int(time);
	media_player->setPosition(0);
	setPlayProgress(0);
}

void MCurveEditor::playbackPosChanged(qint64 time) const
{
	setPlayProgress((double(time) / double(_Duration)) * double(n_frames));
}

void MCurveEditor::on_SvcEditorOpenF0NpyButton_clicked()
{
	if(!ui->F0Widget->graph(0))
	{
		QMessageBox::warning(this, tr("EmptyGraphError"), tr("YouNeedToLoadAAudioFirst"), QMessageBox::Ok);
		return;
	}
	const auto file_path = QFileDialog::getOpenFileName(this, 
		"Open Npy File", to_byte_string(GetCurrentFolder()).c_str(), "Numpy Data File (*.npy);");
	if (file_path.isEmpty())
		return;
	FILE* fp_i = nullptr;
	_wfopen_s(&fp_i, file_path.toStdWString().c_str(), L"rb");
	if (!fp_i)
		return;
	fseek(fp_i, 128, SEEK_SET);
	float F0_Data = 0;
	QVector<double> _F0Data, _F0Key;
	_F0Data.reserve(1000);
	_F0Key.reserve(1000);
	while (fread_s(&F0_Data, sizeof(float), 1, sizeof(float), fp_i) == sizeof(float))
	{
		_F0Key.emplace_back(_F0Data.size());
		_F0Data.emplace_back((12 * log(double(F0_Data) / CenterC) / log(2)) + CenterCPitch);
	}
	if (_F0Data.size() != ui->F0Widget->graph(0)->dataCount())
	{
		QMessageBox::warning(this, tr("F0SizeMisMatchError"), tr("F0SizeMisMatchErrorText"), QMessageBox::Ok);
		return;
	}
	ui->F0Widget->graph(0)->setData(_F0Key, _F0Data);
}