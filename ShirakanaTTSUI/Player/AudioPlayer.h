/*
* file: AudioPlayer.h
* info: 实现音频播放
*
* Author: Maplespe(mapleshr@icloud.com)
*
* date: 2023-2-10 Create
*/
#pragma once
#include "../framework.h"
#include <Render\Sound\Mui_DirectSound.h>

namespace Mui
{
	class UIAudioPlayer
	{
	public:
		UIAudioPlayer(UIWindowBasic* wnd);
		~UIAudioPlayer();

		//暂停当前正在播放的音频
		void Pause();

		//恢复播放 如果播放完毕将重头播放
		void Play();

		//停止播放 播放进度置为0
		void Stop();

		//设置当前音频 如果有音频正在播放 将会停止并替换 进度归零 (wav格式)
		bool SetAudio_Wav(UIResource wav);

		//事件处理 返回false代表事件没有被UIAudioPlayer处理
		bool EventProc(UINotifyEvent event, const UIControl* control, _m_param param);

	private:
		UISlider* m_playbar = nullptr;
		UISlider* m_volbar = nullptr;
		UILabel* m_timeleft = nullptr;
		UILabel* m_timeright = nullptr;
		UILabel* m_volumestr = nullptr;
		UIButton* m_playbtn = nullptr;

		MAudio* m_curAudio = nullptr;
		MAudioTrack* m_track = nullptr;
		MDS_AudioPlayer* m_player = nullptr;

		UIWindowBasic* m_window = nullptr;

		_m_byte m_volume = 80;
		bool m_mute = false;
		bool m_down = false;
		bool is_play = false;

		MTimers::ID m_timer = 0;

		void SetVolume(_m_byte vol);
		static std::wstring formatTime(float time);

		void SetTimer();
		void KillTimer();
		void TimerCallback(_m_param, _m_param, _m_ulong);

		void SetButtonState(bool play) const;
	};
}