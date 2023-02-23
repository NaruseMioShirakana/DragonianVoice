/*
* file: AudioPlayer.cpp
* info: 实现音频播放
*
* Author: Maplespe(mapleshr@icloud.com)
*
* date: 2023-2-10 Create
*/

#include "AudioPlayer.h"
#include <Render\Sound\Mui_AudioWave.h>
#include <sstream>
#include <iomanip>

namespace Mui
{
	UIAudioPlayer::UIAudioPlayer(UIWindowBasic* wnd)
	{
		m_window = wnd;
		m_player = new MDS_AudioPlayer(wnd);
		std::wstring err;
		if (!m_player->InitAudioPlayer(err))
			throw std::runtime_error(M_WStringToString(err));

		m_track = m_player->CreateTrack();

		auto root = wnd->GetRootControl();
		m_playbar = root->Child<UISlider>(L"voice_prog");
		m_volbar = root->Child<UISlider>(L"voice_volume");
		m_timeleft = root->Child<UILabel>(L"voice_timecur");
		m_timeright = root->Child<UILabel>(L"voice_timeall");
		m_volumestr = root->Child<UILabel>(L"volume_text");
		m_playbtn = root->Child<UIButton>(L"voice_play");
		SetButtonState(false);
		m_playbar->SetCurValue(0);
		m_volbar->SetCurValue(80);
		SetVolume(80);
	}

	UIAudioPlayer::~UIAudioPlayer()
	{
		m_window->KillTimer(m_timer);
		m_player->DeleteTrack(m_track);
		delete m_player;
	}

	void UIAudioPlayer::Pause()
	{
		m_player->PauseTrack(m_track);
		KillTimer();
		SetButtonState(false);
	}

	void UIAudioPlayer::Play()
	{
		m_player->PlayTrack(m_track);
		SetTimer();
		SetButtonState(true);
	}

	void UIAudioPlayer::Stop()
	{
		m_player->StopTrack(m_track);
		KillTimer();
		SetButtonState(false);
	}

	bool UIAudioPlayer::SetAudio_Wav(UIResource wav)
	{
		KillTimer();
		//m_player->StopTrack(m_track);
		m_player->SetTrackSound(m_track, nullptr);
		m_curAudio = MCreateAudioWaveFromMem(wav);

		if (!m_curAudio)
			return false;

		const bool ret = m_player->SetTrackSound(m_track, m_curAudio);
		SetVolume(m_volume);

		m_timeleft->SetProperty(UILabel::Label_Text, L"00:00");
		m_timeright->SetProperty(UILabel::Label_Text, formatTime(m_curAudio->GetDuration()));
		m_playbar->SetCurValue(0);
		m_playbar->SetMaxValue(static_cast<_m_uint>(m_curAudio->GetDuration() * 100.0f));
		SetButtonState(false);
		return ret;
	}

	bool UIAudioPlayer::EventProc(UINotifyEvent event, const UIControl* control, _m_param param)
	{
		if (event == Event_Mouse_LClick && control == m_playbtn)
		{
			if (m_playbtn->GetUserData() == 1)
				Pause();
			else
				Play();
			return true;
		}
		else if (event == Event_Slider_Change)
		{
			if (control == m_volbar)
			{
				SetVolume(static_cast<_m_byte>(param));
				m_volumestr->SetProperty(UILabel::Label_Text, std::to_wstring(param) + L"%");
				return true;
			}
			else if (control == m_playbar)
			{
				m_timeleft->SetProperty(UILabel::Label_Text, formatTime((float)param / 100.f));
				return true;
			}
		}
		else if (control == m_playbar)
		{
			if (event == Event_Mouse_LDown && control == m_playbar)
			{
				m_down = true;
				Pause();
				return true;
			}
			else if (event == Event_Mouse_LUp)
			{
				m_down = false;
				const auto value = m_playbar->GetCurValue();
				if (value >= m_playbar->GetMaxValue())
				{
					Pause();
					m_player->SetTrackPlaybackPos(m_track, 0);
					SetButtonState(false);
					return true;
				}
				else
				{
					m_player->SetTrackPlaybackPos(m_track, (float)value / 100.f);
					//Play();
					return true;
				}
			}
		}
		return false;
	}

	void UIAudioPlayer::SetVolume(_m_byte vol)
	{
		m_volume = vol;
		if (m_volume != 0)
		{
			if (m_mute)
			{
				m_mute = false;
				m_player->SetTrackMute(m_track, false);
			}
			m_player->SetTrackVolume(m_track, m_volume);
		}
		else
		{
			m_mute = true;
			m_player->SetTrackMute(m_track, true);
		}
	}

	std::wstring UIAudioPlayer::formatTime(float time)
	{
		const int minutes = int(time) / 60;
		const int seconds = int(time) % 60;
		std::wostringstream oss;
		oss << std::setfill(L'0') << std::setw(2) << minutes << ":" << std::setfill(L'0') << std::setw(2) << seconds;
		return oss.str();
	}

	void UIAudioPlayer::SetTimer()
	{
		m_timer = m_window->SetTimer(40, std::bind(&UIAudioPlayer::TimerCallback, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}

	void UIAudioPlayer::KillTimer()
	{
		m_window->KillTimer(m_timer);
		m_timer = 0;
	}

	void UIAudioPlayer::TimerCallback(_m_param, _m_param, _m_ulong)
	{
		if (m_down || !m_curAudio) return;

		const float cur = m_player->GetTrackPlaybackPos(m_track);
		m_timeleft->SetProperty(UILabel::Label_Text, formatTime(cur));
		m_playbar->SetCurValue(_m_uint(cur * 100));

		if (cur >= m_curAudio->GetDuration())
		{
			Pause();
			m_player->SetTrackPlaybackPos(m_track, 0);
			SetButtonState(false);
		}
	}

	void UIAudioPlayer::SetButtonState(bool play) const
	{
		static_cast<UILabel*>(m_playbtn)->SetProperty(UILabel::Label_Text, play ? L"暂停" : L"播放");
		m_playbtn->SetUserData(play);
	}
}