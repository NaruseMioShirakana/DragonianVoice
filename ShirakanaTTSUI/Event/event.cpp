#include "../Window/MainWindow.h"
#include "../Helper/Helper.h"
#include "../Infer/inferTools.hpp"
#include <fstream>
#include <commdlg.h>
#include <io.h>
#include "../Lib/rapidjson/document.h"
#include "../Lib/rapidjson/writer.h"
#include <regex>
#include "../AvCodeResample.h"
#include <codecvt>
#include "worlds.h"

#ifdef DMLMOESS
#include <dml_provider_factory.h>
#endif
namespace ttsUI
{
	using namespace Mui;

	void MioTTSControl::GetAudio()
	{
		size_t i = 0;
		while (true)
			for (size_t j = 0; j < k_vector.size(); ++j)
				for (size_t k = 0; k < 4; ++k)
				{
					if (!(i < _wavData.size()))
						return;
					const auto tmpCha = (unsigned int(_wavData[i]) >> 2) << 2;
					_wavData[i] = short(int(k_vector[j][k] | tmpCha));
					++i;
				}
	}

	void MioTTSControl::GetUIResourceFromDataChunk()
	{
		GetAudio();
		const _m_size dataSize = static_cast<_m_size>(_wavData.size()) * 4 + 44;

		std::wstring charac = L"None";
		if (modlist_child->GetCurSelItem() != -1)
			charac = modlist_child->GetItem(modlist_child->GetCurSelItem())->GetText();
		std::string METADATA = R"({"Info" : "GeneratedByAI - MoeSS, This Application is Free to use")";
			//,"SrcText" : "
		METADATA += R"(,"Model":")" + to_byte_string(_models[modlist_main->GetCurSelItem()].Name) + R"(")";
		METADATA += R"(,"Type":")" + to_byte_string(_models[modlist_main->GetCurSelItem()].Type) + R"(")";
		METADATA += R"(,"SamplingRate":")" + std::to_string(_models[modlist_main->GetCurSelItem()].Samrate) + R"(")";
		if(_modelType == modelType::Vits || _modelType == modelType::Taco)
		{
			METADATA += R"(,"SrcText":")" + to_byte_string(wave_editbox->GetCurText()) + R"(")";
			if(_modelType == modelType::Vits)
			{
				METADATA += R"(,"NoiseScale":")" + to_byte_string(noise_scale_box->GetCurText()) + R"(")";
				METADATA += R"(,"DurationPredictorNoiseScale":")" + to_byte_string(noise_scale_w_box->GetCurText()) + R"(")";
				METADATA += R"(,"LengthScale":")" + to_byte_string(length_scale_box->GetCurText()) + R"(")";
			}
		}
		if(charac==L"None")
			METADATA += R"(,"Chara":")" + to_byte_string(_models[modlist_main->GetCurSelItem()].Name) + R"(")";
		else
			METADATA += R"(,"Chara":")" + to_byte_string(charac) + R"(")";
		if (_modelType == modelType::diffSinger || _modelType == modelType::diffSvc)
			METADATA += R"(","Seed":")" + to_byte_string(tran_seeds->GetCurText()) + R"("})";

		const auto data = new unsigned char[dataSize + METADATA.size() + 45];
		data[0x0] = 'R'; data[0x1] = 'I'; data[0x2] = 'F'; data[0x3] = 'F';
		*(uint32_t*)(&data[0x4]) = dataSize - 8;
		data[0x8] = 'W'; data[0x9] = 'A'; data[0xa] = 'V'; data[0xb] = 'E';
		data[0xc] = 'f'; data[0xd] = 'm'; data[0xe] = 't'; data[0xf] = ' ';
		*(uint32_t*)(&data[0x10]) = 16; // fmtSize
		*(uint16_t*)(&data[0x14]) = 1; *(uint16_t*)(&data[0x16]) = 2;
		*(uint32_t*)(&data[0x18]) = sampingRate; *(uint32_t*)(&data[0x1c]) = sampingRate * 4;
		*(uint16_t*)(&data[0x20]) = 4; *(uint16_t*)(&data[0x22]) = 16;
		data[0x24] = 'd'; data[0x25] = 'a'; data[0x26] = 't'; data[0x27] = 'a';
		*(uint32_t*)(&data[0x28]) = dataSize - 44;
		const auto dc = (uint16_t*)(&data[0x2c]);
		for (size_t i = 0; i < _wavData.size(); ++i)
		{
			dc[2 * i] = _wavData[i]; dc[2 * i + 1] = _wavData[i];
		}
		memcpy(&data[dataSize + 1], METADATA.c_str(), METADATA.size());
		tmpWav = { data,dataSize + static_cast<_m_size>(METADATA.size()) + 45 };
		_wavData.clear();
	}

	void MioTTSControl::GetUIResourceFromDataChunk(int sr)
	{
		GetAudio();
		const _m_size dataSize = static_cast<_m_size>(_wavData.size()) * 2 + 44;
		const auto data = new unsigned char[dataSize + 45];
		data[0x0] = 'R'; data[0x1] = 'I'; data[0x2] = 'F'; data[0x3] = 'F';
		*(uint32_t*)(&data[0x4]) = dataSize - 8;
		data[0x8] = 'W'; data[0x9] = 'A'; data[0xa] = 'V'; data[0xb] = 'E';
		data[0xc] = 'f'; data[0xd] = 'm'; data[0xe] = 't'; data[0xf] = ' ';
		*(uint32_t*)(&data[0x10]) = 16; // fmtSize
		*(uint16_t*)(&data[0x14]) = 1; *(uint16_t*)(&data[0x16]) = 1;
		*(uint32_t*)(&data[0x18]) = sr; *(uint32_t*)(&data[0x1c]) = sr * 2;
		*(uint16_t*)(&data[0x20]) = 2; *(uint16_t*)(&data[0x22]) = 16;
		data[0x24] = 'd'; data[0x25] = 'a'; data[0x26] = 't'; data[0x27] = 'a';
		*(uint32_t*)(&data[0x28]) = dataSize - 44;
		const auto dc = (uint16_t*)(&data[0x2c]);
		memcpy(&data[0x2c], _wavData.data(), _wavData.size() * 2);
		tmpWav = { data,dataSize };
		_wavData.clear();
	}

	void MioTTSControl::OutPutBatchAudio(const std::wstring& _inputPath)
	{
		std::wstring outPath = GetCurrentFolder() + L"\\OutPuts\\";
		size_t _Off = _inputPath.rfind(L'\\');
		if (_Off == std::wstring::npos)
			_Off = _inputPath.rfind(L'/');
		outPath += _inputPath.substr(_Off + 1) + L' ' + std::to_wstring((int64_t)_inputPath.data()) + L".wav";
		GetUIResourceFromDataChunk();
		DMResources().WriteFiles(outPath, tmpWav);
		tmpWav.Release();
	}

	std::map<std::string, int64_t> midiPitch{
		{"C",0},{"C#",1},{"D",2},{"D#",3},{"E",4},{"F",5},
		{"F#",6},{"G",7},{"G#",8},{"A",9},{"A#",10},{"B",11},
		{"Db",1},{"Eb",3},{"Gb",6},{"Ab",8},{"Bb",10}
	};

	std::wstring charaSets = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	void MioTTSControl::RegisterControl()
	{
		process = _uiControl->Child<UIProgressBar>(L"process");
		titlebar = _uiControl->Child<UIBlurLayer>(L"titlebar");
		title_close = _uiControl->Child<UIButton>(L"title_close");
		title_minisize = _uiControl->Child<UIButton>(L"title_minisize");
		title_label = _uiControl->Child<UILabel>(L"title_label");
		tts_begin = _uiControl->Child<IconButton>(L"tts_begin");
		wave_editbox = _uiControl->Child<UIEditBox>(L"wave_editbox");
		modlist_main = _uiControl->Child<UIComBox>(L"modlist_main");
		modlist_child = _uiControl->Child<UIComBox>(L"modlist_child");
		modlist_refresh = _uiControl->Child<IconButton>(L"modlist_refresh");
		modlist_folder = _uiControl->Child<IconButton>(L"modlist_folder");
		voice_timecur = _uiControl->Child<UILabel>(L"voice_timecur");
		voice_timeall = _uiControl->Child<UILabel>(L"voice_timeall");
		voice_play = _uiControl->Child<UIButton>(L"voice_play");
		voice_save = _uiControl->Child<IconButton>(L"voice_save");
		voice_volume = _uiControl->Child<UISlider>(L"voice_volume");
		volume_text = _uiControl->Child<UILabel>(L"volume_text");
		voice_imgbox = _uiControl->Child<UIImgBox>(L"voice_imgbox");
		voice_imgname = _uiControl->Child<UILabel>(L"voice_imgname");
		voice_prog = _uiControl->Child<UISlider>(L"voice_prog");
		clean_begin = _uiControl->Child<IconButton>(L"clean_begin");
		noise_scale_w_box = _uiControl->Child<UIEditBox>(L"noise_scale_w_box");
		noise_scale_box = _uiControl->Child<UIEditBox>(L"noise_scale_box");
		length_scale_box = _uiControl->Child<UIEditBox>(L"length_scale_box");
		modlist_child2 = _uiControl->Child<UIComBox>(L"modlist_child2");
		tran_seeds = _uiControl->Child<UIEditBox>(L"tran_seeds");
		customPathStat = _uiControl->Child<UICheckBox>(L"customPathStat");
	}

	bool findAll(long* tmpIt, CutInfo& cutInfo, std::wsmatch& _match, const std::wstring& _inputLens)
	{
		if (!std::regex_search(_inputLens, _match, std::wregex(L"([-0-9]+)([, \n\r])")))
			return false;
		*tmpIt = static_cast<long>(_wtoi(_match[1].str().c_str()));
		if ((_match[2].str() != L"," && _match[2].str() != L" ") || tmpIt == &cutInfo.frame_shift)
			return true;
		findAll(tmpIt + 1, cutInfo, _match, std::wstring(_match[0].second, _inputLens.end()));
		return false;
	}

	const std::wregex pathSymbol(L"[/]");

	std::vector<std::wstring> CutLens(const std::wstring& input)
	{
		std::vector<std::wstring> _Lens;
		std::wstring _tmpLen;
		for (const auto& chari : input)
		{
			if ((chari == L'\n') || (chari == L'\r')) {
				if (!_tmpLen.empty())
				{
					_Lens.push_back(_tmpLen);
					_tmpLen = L"";
				}
			}
			else {
				_tmpLen += chari;
			}
		}
		return _Lens;
	}

	std::vector<std::vector<std::wstring>> MioTTSControl::GetParam(std::vector<std::wstring>& input) const
	{
		std::vector<std::vector<std::wstring>> _Lens;
		_Lens.reserve(input.size());
		std::wsmatch match_results;
		for (auto& chari : input)
		{
			std::vector<std::wstring> Param = { noise_scale_box->GetCurText(),noise_scale_w_box->GetCurText(),length_scale_box->GetCurText(),tran_seeds->GetCurText(),std::to_wstring(modlist_child->GetCurSelItem()),L"0" };
			if (std::regex_search(chari, match_results, NoiseScaleParam))
				Param[0] = match_results[2];
			if (std::regex_search(chari, match_results, NoiseScaleWParam))
				Param[1] = match_results[2];
			if (std::regex_search(chari, match_results, LengthScaleParam))
				Param[2] = match_results[2];
			if (std::regex_search(chari, match_results, SeedParam))
				Param[3] = match_results[2];
			if (std::regex_search(chari, match_results, CharaParam))
				Param[4] = match_results[2];
			if (std::regex_search(chari, match_results, EmoParam))
				Param[5] = match_results[2];
			const auto iter = chari.rfind(EndString);
			if (iter != std::wstring::npos)
				chari = chari.substr(iter + 2);
			_Lens.push_back(std::move(Param));
		}
		return _Lens;
	}

	std::map<std::wstring, std::vector<std::wstring>> GetPhonesPairMap(const std::wstring& path)
	{
		std::string phoneInfo, phoneInfoAll;
		std::ifstream phonefile(path.c_str());
		if (!phonefile.is_open())
			throw std::exception("phone file not found");
		while (std::getline(phonefile, phoneInfo))
			phoneInfoAll += phoneInfo;
		phonefile.close();
		rapidjson::Document PhoneJson;
		PhoneJson.Parse(phoneInfoAll.c_str());
		if (PhoneJson.HasParseError())
			throw std::exception("json file error");
		std::map<std::wstring, std::vector<std::wstring>> TmpOut;
		for (auto itr = PhoneJson.MemberBegin(); itr != PhoneJson.MemberEnd(); ++itr)
		{
			std::wstring Key = to_wide_string(itr->name.GetString());
			const auto Value = itr->value.GetArray();
			TmpOut[Key] = std::vector<std::wstring>();
			for (const auto& it : Value)
				TmpOut[Key].push_back(to_wide_string(it.GetString()));
		}
		return TmpOut;
	}

	std::map<std::wstring, int64_t> GetPhones(const std::map<std::wstring, std::vector<std::wstring>>& PhonesPair)
	{
		std::map<std::wstring, int64_t> tmpMap;

		for (const auto& it : PhonesPair)
			for (const auto& its : it.second)
				tmpMap[its] = 0;
		tmpMap[L"SP"] = 0;
		tmpMap[L"AP"] = 0;
		int64_t pos = 3;
		for (auto& it : tmpMap)
			it.second = pos++;
		tmpMap[L"PAD"] = 0;
		tmpMap[L"EOS"] = 1;
		tmpMap[L"UNK"] = 2;
		return tmpMap;
	}

	std::wregex tokenReg(L"[A-Za-z]+");
	std::regex pitchReg("[A-Za-z0-9#/]+");
	std::regex noteReg("([A-Za-z#]+)([0-9]+)");
	std::regex numReg("[0-9\\.]+");

	static std::vector<double> arange(double start, double end, double step = 1.0, double div = 1.0)
	{
		std::vector<double> output;
		while (start < end)
		{
			output.push_back(start / div);
			start += step;
		}
		return output;
	}

	std::vector<DiffSingerInput> MioTTSControl::preprocessDiffSinger(const std::vector<std::wstring>& Jsonpath)
	{
		std::vector<DiffSingerInput> OutS;
		OutS.reserve(Jsonpath.size() + 1);
		for(const auto & itPath : Jsonpath)
		{
			std::string phoneInfo, phoneInfoAll;
			std::ifstream phonefile(itPath.c_str());
			if (!phonefile.is_open())
				throw std::exception("phone file not found");
			while (std::getline(phonefile, phoneInfo))
				phoneInfoAll += phoneInfo;
			phonefile.close();
			rapidjson::Document PhoneJson;
			PhoneJson.Parse(phoneInfoAll.c_str());
			if (PhoneJson.HasParseError())
				throw std::exception("json file error");

			DiffSingerInput tmpOut;
			try
			{
				std::wsmatch match_results;
				std::smatch amatch_results;
				std::smatch pitch_results;
				for (auto& it : PhoneJson.GetArray())
				{
					if (it["input_type"].Empty() || it["offset"].IsNull())
						throw std::exception("mis input_type or offset");
					std::string input_type = it["input_type"].GetString();
					size_t reqLen = 0;
					const double frame_length = static_cast<double>(curHop) / static_cast<double>(sampingRate);


					// Tokens Preprocess
					std::wstring rtext;
					std::string phone;
					std::vector<int64_t> Tokens;
					if (input_type == "text")
					{
						throw std::exception("TODO");
						/*
						if (it["text"].Empty())
							throw std::exception("error");
						auto text = std::regex_replace(to_wide_string(it["text"].GetString()), std::wregex(L"SP"), L",");
						text = std::regex_replace(text, std::wregex(L"AP"), L"!");
						text = pluginApi.functionAPI(text);
						text = std::regex_replace(text, std::wregex(L"!"), L"AP");
						text = std::regex_replace(text, std::wregex(L","), L"SP");
						while (std::regex_search(text, match_results, tokenReg))
						{
							if (PhonesPair.find(match_results[0].str()) != PhonesPair.end())
								for (const auto& its : PhonesPair[match_results[0]])
									Tokens.push_back(Phones[its]);
							else
								Tokens.push_back(Phones[match_results[0].str()]);
							text = match_results.suffix();
						}
						 */
					}
					else
					{
						if (it["ph_seq"].Empty())
							throw std::exception("Missing ph_seq");
						if (!MidiVer)
						{
							if (it["ph_dur"].Empty())
								throw std::exception("Missing ph_dur");
							if(!it["tran_method"].Empty())
							{
								auto pttxt = to_wide_string(it["ph_seq"].GetString()) + L"<<<<DURATION>>>>" + to_wide_string(it["ph_dur"].GetString());
								if (std::string(it["tran_method"].GetString()) == "ChineseToJapanese")
									pttxt = tranTokens::ChineseToJapanese(pttxt);
								else if (std::string(it["tran_method"].GetString()) == "JapaneseToChinese")
									pttxt = tranTokens::JapaneseToChinese(pttxt);
								rtext = pttxt.substr(0, pttxt.find(L"<<<<DURATION>>>>") - 1);
								phone = to_byte_string(pttxt.substr(pttxt.find(L"<<<<DURATION>>>>") + 16));
							}
						}
						std::wstring text = rtext;
						if (text.empty())
							text = to_wide_string(it["ph_seq"].GetString());
						while (std::regex_search(text, match_results, tokenReg))
						{
							if (PhonesPair.find(match_results[0].str()) != PhonesPair.end())
								for (const auto& its : PhonesPair[match_results[0]])
									Tokens.push_back(Phones[its]);
							else if (Phones.find(match_results[0].str()) != Phones.end())
								Tokens.push_back(Phones[match_results[0].str()]);
							else
								throw std::exception(("Unsupported Ph, In Len:" + std::string(it["ph_seq"].GetString())).c_str());
							text = match_results.suffix();
						}
					}
					tmpOut.offset.push_back(it["offset"].GetDouble());


					// Duration Preprocess
					if (!it["ph_dur"].Empty())
					{
						std::vector<double> ph_dur;
						ph_dur.reserve(Tokens.size());
						std::string phstr = phone;
						if (phstr.empty())
							phstr = it["ph_dur"].GetString();
						while (std::regex_search(phstr, amatch_results, numReg))
						{
							ph_dur.push_back(atof(amatch_results[0].str().c_str()));
							phstr = amatch_results.suffix();
						}
						if (ph_dur.size() != Tokens.size())
							throw std::exception("size mismatch ph_dur & Tokens");
						std::vector<int64_t> duration(ph_dur.size(), 0);
						for (size_t i = 1; i < ph_dur.size(); ++i)
							ph_dur[i] += ph_dur[i - 1];
						for (size_t i = 0; i < ph_dur.size(); ++i)
							ph_dur[i] = round((ph_dur[i] / frame_length) + 0.5);
						for (size_t i = ph_dur.size() - 1; i; --i)
							reqLen += (duration[i] = static_cast<int64_t>(ph_dur[i] - ph_dur[i - 1]));
						duration[0] = static_cast<int64_t>(ph_dur[0]);
						reqLen += duration[0];

						tmpOut.durations.push_back(std::move(duration));
					}


					// note Preprocess
					if(MidiVer)
					{
						if (!it["note_seq"].Empty())
						{
							std::vector<int64_t> note_seq;
							note_seq.reserve(Tokens.size());
							std::string notestr = it["note_seq"].GetString();
							while (std::regex_search(notestr, amatch_results, pitchReg))
							{
								auto pitchString = amatch_results[0].str();
								int64_t pitch = atoll(pitchString.c_str());
								if (!pitch)
								{
									if (std::regex_search(pitchString, pitch_results, noteReg))
										pitch = 12 + midiPitch[pitch_results[1].str()] + 12 * atoll(pitch_results[2].str().c_str());
									else
										pitch = 0;
								}
								note_seq.push_back(pitch);
								notestr = amatch_results.suffix();
							}
							if (note_seq.size() != Tokens.size())
								throw std::exception("size mismatch note_seq & Tokens");
							tmpOut.pitchs.push_back(std::move(note_seq));
						}
						if (!it["note_dur_seq"].Empty())
						{
							std::vector<int64_t> note_duration(Tokens.size(), 0);
							std::vector<double> note_dur_seq;
							std::string notestr = it["note_dur_seq"].GetString();
							while (std::regex_search(notestr, amatch_results, numReg))
							{
								note_dur_seq.push_back(atof(amatch_results[0].str().c_str()));
								notestr = amatch_results.suffix();
							}
							if (note_dur_seq.size() != Tokens.size())
								throw std::exception("size mismatch note_dur_seq & Tokens");
							for (size_t i = 1; i < note_dur_seq.size(); ++i)
								note_dur_seq[i] += note_dur_seq[i - 1];
							for (size_t i = 0; i < note_dur_seq.size(); ++i)
								note_dur_seq[i] = round((note_dur_seq[i] / frame_length) + 0.5);
							for (size_t i = note_dur_seq.size() - 1; i; --i)
								note_duration[i] = static_cast<int64_t>(note_dur_seq[i] - note_dur_seq[i - 1]);
							note_duration[0] = static_cast<int64_t>(note_dur_seq[0]);
							tmpOut.pitch_durations.push_back(std::move(note_duration));
						}
					}

					
					//f0 Preprocess
					if (!it["f0_seq"].Empty())
					{
						if(it["f0_timestep"].IsNull())
							throw std::exception("mis f0_timestep");
						std::vector<double> f0_seq;
						std::string f0str = it["f0_seq"].GetString();
						while (std::regex_search(f0str, amatch_results, numReg))
						{
							f0_seq.push_back(atof(amatch_results[0].str().c_str()));
							f0str = amatch_results.suffix();
						}
						double f0_timestep = atof(it["f0_timestep"].GetString());
						const double t_max = static_cast<double>(f0_seq.size() - 1) * f0_timestep;
						const auto x0 = arange(0.0, static_cast<double>(f0_seq.size()), 1.0, (1.0 / f0_timestep));
						auto xi = arange(0.0, t_max, frame_length);
						while (xi.size() < reqLen)
							xi.push_back(*(xi.end() - 1) + f0_timestep);
						while (xi.size() > reqLen)
							xi.pop_back();
						std::vector<double> yi(xi.size(), 0.0);
						interp1(x0.data(), f0_seq.data(), static_cast<int>(x0.size()), xi.data(), static_cast<int>(xi.size()), yi.data());
						std::vector<float> f0(xi.size(), 0.0);
						for (size_t i = 0; i < yi.size(); ++i)
							f0[i] = static_cast<float>(yi[i]);
						tmpOut.f0.push_back(std::move(f0));
					}


					tmpOut.inputLens.push_back(std::move(Tokens));
				}
			}
			catch (std::exception& e)
			{
				throw std::exception(e.what());
			}
			OutS.push_back(std::move(tmpOut));
		}
		return OutS;
	}

	bool PreProcessVitsInput(const HWND& hWnd,std::vector<std::wstring>& _Lens, std::map<wchar_t, int64_t>& _symbol)
	{
		for (size_t i = 0; i < _Lens.size(); ++i)
		{
			for (size_t j = 0; j < _Lens[i].length(); ++j)
				if (_symbol.find(_Lens[i][j]) == _symbol.end() && _Lens[i][j] != L'\n' && _Lens[i][j] != L'\r')
				{
					_Lens.clear();
					MessageBox(hWnd,
						(L"字符位置：Len[" +
							std::to_wstring(i + 1) +
							L"] " + L"Index[" +
							std::to_wstring(j) +
							L"]").c_str(),
						L"不支持的字符",
						MB_OK);
					return false;
				}
			if (_Lens[i][_Lens[i].length() - 1] != L'.' || _Lens[i][_Lens[i].length() - 1] != L'?' || _Lens[i][_Lens[i].length() - 1] != L'!')
				_Lens[i] += L'.';
		}
		return true;
	}

	bool InsertMessageToEmptyEditBox(const HWND& hWnd,const modelType& _modelType, std::wstring & _inputLens)
	{
		if (_modelType == modelType::Taco || _modelType == modelType::Vits )
		{
			MessageBox(hWnd, L"请输入要转换的字符串", L"未找到输入", MB_OK);
			return false;
		}
		std::vector<TCHAR> szFileName(MaxPath);
		std::vector<TCHAR> szTitleName(MaxPath);
		OPENFILENAME  ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lpstrFile = szFileName.data();
		ofn.nMaxFile = MaxPath;
		ofn.lpstrFileTitle = szTitleName.data();
		ofn.nMaxFileTitle = MaxPath;
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = nullptr;
		if (_modelType == modelType::SoVits || _modelType == modelType::diffSvc)
		{
			constexpr TCHAR szFilter[] = TEXT("音频 (*.wav;*.mp3;*.ogg;*.flac;*.aac)\0*.wav;*.mp3;*.ogg;*.flac;*.aac\0");
			ofn.lpstrFilter = szFilter;
			ofn.lpstrTitle = L"打开音频";
			ofn.lpstrDefExt = TEXT("wav");
			ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			if (GetOpenFileName(&ofn))
			{
				auto filePtr = szFileName.data();
				std::wstring preFix = filePtr;
				filePtr += preFix.length() + 1;
				if (!*filePtr)
					_inputLens = preFix;
				else
				{
					preFix += L'\\';
					while (*filePtr != 0)
					{
						std::wstring thisPath(filePtr);
						_inputLens += preFix + thisPath + L'\n';
						filePtr += thisPath.length() + 1;
					}
				}
			}
			else
			{
				MessageBox(hWnd, L"请输入要转换的文件", L"未找到输入", MB_OK);
				return false;
			}
		}
		else
		{
			constexpr TCHAR szFilter[] = TEXT("DiffSinger项目文件 (*.json;*.ds)\0*.json;*.ds\0");
			ofn.lpstrFilter = szFilter;
			ofn.lpstrTitle = L"打开项目";
			ofn.lpstrDefExt = TEXT("json");
			ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			if (GetOpenFileName(&ofn))
			{
				auto filePtr = szFileName.data();
				std::wstring preFix = filePtr;
				filePtr += preFix.length() + 1;
				if (!*filePtr)
					_inputLens = preFix;
				else
				{
					preFix += L'\\';
					while (*filePtr != 0)
					{
						std::wstring thisPath(filePtr);
						_inputLens += preFix + thisPath + L'\n';
						filePtr += thisPath.length() + 1;
					}
				}
			}
			else
			{
				MessageBox(hWnd, L"请输入要转换的文件", L"未找到输入", MB_OK);
				return false;
			}
		}
		return true;
	}

#ifdef DMLMOESS
	static void ORT_API_CALL LoggingFunction(void* /*param*/, OrtLoggingLevel severity, const char* /*category*/, const char* /*logid*/, const char* /*code_location*/, const char* message)
	{
		printf("onnxruntime: %s\n", message);
	}
#endif

	void MioTTSControl::RegisterEvent()
	{
		RegisterLeftClickEvent
		(
			L"title_close",
			[&]()
			{
				::SendMessageW((HWND)(_mainWindow->GetWindowHandle()), M_WND_CLOSE, 0, 0);
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"title_minisize",
			[&]()
			{
				::ShowWindow(hWnd, SW_MINIMIZE);
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"modlist_refresh",
			[&]()
			{
				setConsDefalut();
				loadmodel();
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"tts_begin",
			[&]()
			{
				_mainWindow->m_player->Pause();
				std::thread inferThread(
					[&]()
					{
						clean_begin->SetEnabled(false);
						tts_begin->SetEnabled(false);
						modlist_main->SetEnabled(false);
						modlist_refresh->SetEnabled(false);
						auto _inputLens = wave_editbox->GetCurText();
						while (_inputLens.back() == L'\n' || _inputLens.back() == L'\r')
							_inputLens.pop_back();
						if (_inputLens.length() == 0)
							if (!InsertMessageToEmptyEditBox(hWnd, _modelType, _inputLens))
							{
								InferQuit();
								return;
							}
						_mainWindow->m_player->Stop();
						tmpWav.Release();
						voice_prog->SetCurValue(0);
						voice_timecur->SetAttribute(L"text", L"00:00");
						voice_timeall->SetAttribute(L"text", L"00:00");
						setSoundModule(false);
						process->SetVisible(true);
						std::vector<std::vector<std::wstring>> Params;
						_inputLens += L'\n';
						std::vector<std::wstring> _Lens = CutLens(_inputLens);
						if (_modelType == modelType::Vits || enableTModel)
							Params = GetParam(_Lens);
						if (!(_modelType == modelType::SoVits || _modelType == modelType::diffSvc || _modelType == modelType::diffSinger) || enableTModel)
						{
							if (!PreProcessVitsInput(hWnd, _Lens, _symbol))
							{
								InferQuit();
								return;
							}
							process->SetAttribute(L"max", std::to_wstring(_Lens.size()));
						}
						_wavData.clear();
						int proc = 0;
						auto _chara = modlist_child->GetCurSelItem();
						std::vector<DiffSingerInput> _diffinput;
						if (_modelType == modelType::diffSinger)
						{
							try
							{
								_diffinput = preprocessDiffSinger(_Lens);
							}
							catch (std::exception& e)
							{
								MessageBox(hWnd, (L"输入错误" + to_wide_string(e.what())).c_str(), L"错误", MB_OK);
								InferQuit();
								return;
							}
						}
						try
						{
							DiffusionInfo finfo;
							CutInfo cutInfo;
							auto callback = [&](const VcConfigLayer::CInfo& input)
							{
								cutInfo.threshold = input.threshold;
								cutInfo.minLen = input.minLen;
								cutInfo.keys = input.keys;
								cutInfo.frame_shift = input.frame_shift;
								cutInfo.frame_len = input.frame_len;
								if (V2)
								{
									finfo.pndm = input.pndm;
									finfo.step = input.step;
								}
								_mainWindow->vc_layer->opened = false;
							};
							switch (_modelType)
							{
							case modelType::Taco:
							{
								for (const std::wstring& _it : _Lens)
								{
									InferTaco2(_it);
									process->SetAttribute(L"value", std::to_wstring(proc++));
								}
								break;
							}
							case modelType::Vits:
							{
								if (sessionEmb && _chara == -1)
								{
									MessageBox(hWnd, L"请指定一个角色", L"未指定角色", MB_OK);
									InferQuit();
									return;
								}
								InferVits(_Lens, Params);
								break;
							}
							case modelType::SoVits:
							{
								if (_chara == -1)
								{
									MessageBox(hWnd, L"请指定一个角色", L"未指定角色", MB_OK);
									InferQuit();
									return;
								}
								_mainWindow->vc_layer->ShowLayer(L"设置SVC参数", false, callback);
								while (_mainWindow->vc_layer->opened)
									Sleep(200);
								if (_mainWindow->vc_layer->isCanceled())
								{
									InferQuit();
									return;
								}
								if (!enableTModel)
								{
									std::wstring _tmpLen;
									for (auto& _inputPath : _Lens)
									{
										_inputPath = std::regex_replace(_inputPath, pathSymbol, L"\\");
										inferSovits(_inputPath, cutInfo, _chara);
										if (_Lens.size() > 1)
										{
											OutPutBatchAudio(_inputPath);
											_wavData.clear();
										}
									}
									if (_Lens.size() > 1)
									{
										InferQuit();
										return;
									}
								}
								else
									inferSovitsT(_Lens, Params, cutInfo, _chara);
								break;
							}
							case modelType::diffSvc:
							{
								_mainWindow->vc_layer->ShowLayer(L"设置SVC参数", V2, callback);
								while (_mainWindow->vc_layer->opened)
									Sleep(200);
								if (_mainWindow->vc_layer->isCanceled())
								{
									InferQuit();
									return;
								}
								if (_chara == -1)
									_chara = 0;
								if (!enableTModel)
								{
									std::wstring _tmpLen;
									for (auto& _inputPath : _Lens)
									{
										_inputPath = std::regex_replace(_inputPath, pathSymbol, L"\\");
										if (V2)
											inferDiff(_inputPath, cutInfo, _chara, finfo);
										else
											inferSvc(_inputPath, cutInfo, _chara);
										if (_Lens.size() > 1)
										{
											OutPutBatchAudio(_inputPath);
											_wavData.clear();
										}
									}
									if (_Lens.size() > 1)
									{
										InferQuit();
										return;
									}
								}
								else
									inferSvcT(_Lens, Params, cutInfo, _chara, finfo);
								break;
							}
							case modelType::diffSinger:
							{
								V2 = true;
								_mainWindow->vc_layer->ShowLayer(L"设置Singer参数", true, callback);
								while (_mainWindow->vc_layer->opened)
									Sleep(200);
								if (_mainWindow->vc_layer->isCanceled())
								{
									InferQuit();
									return;
								}
								if (_chara == -1)
									_chara = 0;
								std::wstring _tmpLen;
								int curAmount = 0;
								for (auto& diff_inputs : _diffinput)
								{
									if(diffSinger)
										inferDiffSinger2(diff_inputs, cutInfo, _chara, finfo);
									else
										inferDiffSinger(diff_inputs, cutInfo, _chara, finfo);
									if (_diffinput.size() > 1)
									{
										const auto _inputPath = _Lens[curAmount];
										OutPutBatchAudio(_inputPath);
										_wavData.clear();
										++curAmount;
									}
								}
								if (_Lens.size() > 1)
								{
									InferQuit();
									return;
								}
								break;
							}
							}
						}
						catch (std::exception& e)
						{
							MessageBox(hWnd, to_wide_string(e.what()).c_str(), L"错误", MB_ICONERROR | MB_OK);
							InferQuit();
							_wavData.clear();
							return;
						}
						GetUIResourceFromDataChunk();
						_wavData.clear();
						process->SetAttribute(L"value", std::to_wstring(proc));
						Sleep(500);
						InferQuit();
						if (!tmpWav.data)
							MessageBox(hWnd, L"创建Temp失败", L"错误", MB_OK);
						else
						{
							if(!_mainWindow->m_player->SetAudio_Wav(tmpWav))
								MessageBox(hWnd, L"创建Temp失败", L"错误", MB_OK);
							else
								setSoundModule(true);
						}
					}
				);
				inferThread.detach();
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"clean_begin",
			[&]()
			{
				std::wstring Temp = wave_editbox->GetCurText();
				wave_editbox->SetCurText(getCleanerStr(Temp));
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"voice_save",
			[&]()
			{
				TCHAR  szFileName[MAX_PATH] = { 0 };
				TCHAR szTitleName[MAX_PATH] = { 0 };
				constexpr TCHAR szFilter[] = TEXT("波形音频 (*.wav)\0*.wav\0");
				OPENFILENAME  ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFileTitle = szTitleName;
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.lpstrFilter = szFilter;
				ofn.lpstrDefExt = TEXT("wav");
				ofn.lpstrTitle = L"另存为";
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = nullptr;
				if (GetSaveFileName(&ofn))
				{
					if (std::wstring(szFileName).find(L".wav") == std::wstring::npos)
					{
						MessageBox(hWnd, L"路径错误", L"保存失败", MB_OK);
						return true;
					}
					DeleteFile(szFileName);
					DMResources().WriteFiles(szFileName, tmpWav);
					//CopyFile((GetCurrentFolder() + L"\\temp\\tmp.wav").c_str(), szFileName, true);
				}
				return true;
			}
		);
		RegisterLeftClickEvent
		(
			L"modlist_folder",
			[]()
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				ZeroMemory(&si, sizeof(STARTUPINFO));
				ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
				si.cb = sizeof(STARTUPINFO);
				CreateProcess(nullptr,
					const_cast<LPWSTR>((L"Explorer.exe " + GetCurrentFolder() + L"\\Mods").c_str()),
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
				return true;
			}
		);
		RegisterValEvent
		(
			L"modlist_main",
			[&](uint64_t _index)
			{
				ClearModel();
				if (_index == modlist_main->GetItemListCount() - 1)
				{
					voice_imgname->SetAttribute(L"text", L"当前未选中模型");
					const auto defaultBg = DMResources().ReadFiles(GetCurrentFolder() + L"\\default.png");
					if (defaultBg.data)
						voice_imgbox->SetImage(defaultBg);
					return true;
				}
#ifdef CUDAMOESS
				const auto AvailableProviders = Ort::GetAvailableProviders();
				bool ret = true;
				for (const auto& it : AvailableProviders)
					if (it.find("CUDA") != std::string::npos)
						ret = false;
				if (ret)
				{
					MessageBox(hWnd, L"加载模型失败，不存在CUDA Provider", L"失败", MB_OK);
					ClearModel();
					modlist_main->SetCurSelItem(-1);
					return true;
				}
				OrtCUDAProviderOptions cuda_option;
				cuda_option.device_id = 0;
#endif
#ifdef DMLMOESS
				const auto AvailableProviders = Ort::GetAvailableProviders();
				std::string ret;
				for (const auto& it : AvailableProviders)
					if (it.find("Dml") != std::string::npos)
						ret = it;
				if (ret.empty())
				{
					MessageBox(hWnd, L"加载模型失败，不存在DML Provider", L"失败", MB_OK);
					ClearModel();
					modlist_main->SetCurSelItem(-1);
					return true;
				}
				const OrtApi& ortApi = Ort::GetApi();
				const OrtDmlApi* ortDmlApi = nullptr;
				ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi));
#endif
				session_options = new Ort::SessionOptions;
#ifdef CUDAMOESS
				env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
				session_options->AppendExecutionProvider_CUDA(cuda_option);
				session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
				session_options->SetIntraOpNumThreads(1);
				memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
#else
#ifdef DMLMOESS
				ThreadingOptions threadingOptions;
				env = new Ort::Env(threadingOptions.threadingOptions, LoggingFunction, nullptr, ORT_LOGGING_LEVEL_VERBOSE, "");
				env->DisableTelemetryEvents();
				ortDmlApi->SessionOptionsAppendExecutionProvider_DML(*session_options, 0);
				session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
				session_options->DisablePerSessionThreads();
				session_options->SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
				session_options->DisableMemPattern();
				memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU));
#else
				env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "OnnxModel");
				session_options->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency()));
				session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
				memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
#endif
#endif
				
				const std::wstring _path = GetCurrentFolder() + L"\\Mods\\" + _models[_index].Folder + L"\\" + _models[_index].Folder;
				if (_models[_index].Type == L"Tacotron2")
				{
					_modelType = modelType::Taco;
					try
					{
						sessionEncoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
						sessionDecoderIter = new Ort::Session(*env, (_path + L"_decoder_iter.onnx").c_str(), *session_options);
						sessionPostNet = new Ort::Session(*env, (_path + L"_postnet.onnx").c_str(), *session_options);
						sessionGan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + _models[_index].Hifigan + L".onnx").c_str(), *session_options);
						memory_info = new Ort::MemoryInfo(Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault));
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误\n" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
				}
				else if (_models[_index].Type == L"Vits")
				{
					_modelType = modelType::Vits;
					try
					{
						sessionDec = new Ort::Session(*env, (_path + L"_dec.onnx").c_str(), *session_options);
						sessionDp = new Ort::Session(*env, (_path + L"_dp.onnx").c_str(), *session_options);
						sessionEnc_p = new Ort::Session(*env, (_path + L"_enc_p.onnx").c_str(), *session_options);
						sessionFlow = new Ort::Session(*env, (_path + L"_flow.onnx").c_str(), *session_options);
						if (_waccess((_path + L"_emb.onnx").c_str(), 0) != -1)
							sessionEmb = new Ort::Session(*env, (_path + L"_emb.onnx").c_str(), *session_options);
						else
							sessionEmb = nullptr;
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					for (auto& it : _models[_index].Speakers)
					{
						const auto _tmpItem = new ListItem;
						_tmpItem->SetText(L"  " + it);
						modlist_child->AddItem(_tmpItem, -1);
					}
					length_scale_box->SetEnabled(true);
					noise_scale_box->SetEnabled(true);
					noise_scale_w_box->SetEnabled(true);
					tran_seeds->SetEnabled(true);
					if (!_models[_index].Speakers.empty())
					{
						modlist_child->SetEnabled(true);
						modlist_child->SetCurSelItem(0);
					}
					emo = _models[_index].emo;
					try {
						if (emo)
						{
							const auto emopath = GetCurrentFolder() + L"\\emotion\\" + _models[_index].emoPath + L".npy";
							const auto emopathdef = GetCurrentFolder() + L"\\emotion\\" + _models[_index].emoPath + L".json";
							emoLoader.open(emopath);
							emoStringa.clear();
							EmoJson.Clear();
							std::ifstream EmoFiles(emopathdef.c_str());
							if (EmoFiles.is_open())
							{
								std::string JsonLine;
								while (std::getline(EmoFiles, JsonLine))
									emoStringa += JsonLine;
								EmoFiles.close();
								EmoJson.Parse(emoStringa.c_str());
							}
						}
					}
					catch (std::exception& e)
					{
						MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(e.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
				}
				else if (_models[_index].Type == L"SoVits")
				{
					_modelType = modelType::SoVits;
					try
					{
						hubert = new Ort::Session(*env, (GetCurrentFolder() + L"\\hubert\\" + _models[_index].Hubert + L".onnx").c_str(), *session_options);
						soVits = new Ort::Session(*env, (_path + L"_SoVits.onnx").c_str(), *session_options);
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					for (auto& it : _models[_index].Speakers)
					{
						const auto _tmpItem = new ListItem;
						_tmpItem->SetText(L"  " + it);
						modlist_child->AddItem(_tmpItem, -1);
					}
					const auto _tmpItem1 = new ListItem;
					_tmpItem1->SetText(L"  None");
					modlist_child2->AddItem(_tmpItem1, -1);
					enableTModel = false;
					for (auto& it : _models)
					{
						if (it.Type == L"Vits")
						{
							TModelFolder.push_back(it.Folder);
							const auto _tmpItem = new ListItem;
							_tmpItem->SetText(L"  " + it.Folder);
							modlist_child2->AddItem(_tmpItem, -1);
						}
					}
					modlist_child2->SetEnabled(true);
					modlist_child2->SetCurSelItem(0);
					modlist_child->SetEnabled(true);
					modlist_child->SetCurSelItem(0);
					curHop = _models[_index].hop;
					Sv3 = _models[_index].SV3;
					Sv4 = _models[_index].SV4;
					if(Sv3 && Sv4) 
					{
						MessageBox(hWnd, L"模型配置文件有误", L"错误！", MB_OK);
						modlist_main->SetCurSelItem(static_cast<int>(_models.size()) - 1);
						ClearModel();
						return true;
					}
					if(Sv4)
					{
						noise_scale_box->SetEnabled(true);
						tran_seeds->SetEnabled(true);
					}
				}
				else if (_models[_index].Type == L"DiffSvc")
				{
					_modelType = modelType::diffSvc;
					V2 = _models[_index].V2;
					try
					{
						hubert = new Ort::Session(*env, (GetCurrentFolder() + L"\\hubert\\" + _models[_index].Hubert + L".onnx").c_str(), *session_options);
						nsfHifigan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + _models[_index].Hifigan + L".onnx").c_str(), *session_options);
						if (V2)
						{
							encoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
							denoise = new Ort::Session(*env, (_path + L"_denoise.onnx").c_str(), *session_options);
							pred = new Ort::Session(*env, (_path + L"_pred.onnx").c_str(), *session_options);
							after = new Ort::Session(*env, (_path + L"_after.onnx").c_str(), *session_options);
						}
						else
							diffSvc = new Ort::Session(*env, (_path + L"_DiffSvc.onnx").c_str(), *session_options);
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					if (!_models[_index].Speakers.empty())
					{
						for (auto& it : _models[_index].Speakers)
						{
							const auto _tmpItem = new ListItem;
							_tmpItem->SetText(L"  " + it);
							modlist_child->AddItem(_tmpItem, -1);
						}
						modlist_child->SetEnabled(true);
						modlist_child->SetCurSelItem(0);
					}
					curHop = _models[_index].hop;
					curPndm = _models[_index].pndm;
					tran_seeds->SetEnabled(true);
					curMelBins = _models[_index].melBins;
					const auto _tmpItem1 = new ListItem;
					_tmpItem1->SetText(L"  None");
					modlist_child2->AddItem(_tmpItem1, -1);
					enableTModel = false;
					for (auto& it : _models)
					{
						if (it.Type == L"Vits")
						{
							TModelFolder.push_back(it.Folder);
							const auto _tmpItem = new ListItem;
							_tmpItem->SetText(L"  " + it.Folder);
							modlist_child2->AddItem(_tmpItem, -1);
						}
					}
					modlist_child2->SetEnabled(true);
					modlist_child2->SetCurSelItem(0);
				}
				else if (_models[_index].Type == L"DiffSinger")
				{
					_modelType = modelType::diffSinger;
					try
					{
#ifndef CUDAMOESS
						nsfHifigan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + _models[_index].Hifigan + L".onnx").c_str(), *session_options);
#else
						session_options_nsf = new Ort::SessionOptions;
						session_options_nsf->SetIntraOpNumThreads(static_cast<int>(std::thread::hardware_concurrency()));
						session_options_nsf->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
						nsfHifigan = new Ort::Session(*env, (GetCurrentFolder() + L"\\hifigan\\" + _models[_index].Hifigan + L".onnx").c_str(), *session_options_nsf);
#endif
						if (_waccess((_path + L"_DiffSinger.onnx").c_str(), 0) != -1)
							diffSinger = new Ort::Session(*env, (_path + L"_DiffSinger.onnx").c_str(), *session_options);
						else
						{
							encoder = new Ort::Session(*env, (_path + L"_encoder.onnx").c_str(), *session_options);
							denoise = new Ort::Session(*env, (_path + L"_denoise.onnx").c_str(), *session_options);
							pred = new Ort::Session(*env, (_path + L"_pred.onnx").c_str(), *session_options);
							after = new Ort::Session(*env, (_path + L"_after.onnx").c_str(), *session_options);
						}
					}
					catch (Ort::Exception& _exception)
					{
						MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					if (!_models[_index].Speakers.empty())
					{
						for (auto& it : _models[_index].Speakers)
						{
							const auto _tmpItem = new ListItem;
							_tmpItem->SetText(L"  " + it);
							modlist_child->AddItem(_tmpItem, -1);
						}
						modlist_child->SetEnabled(true);
						modlist_child->SetCurSelItem(0);
					}
					tran_seeds->SetEnabled(true);
					curMelBins = _models[_index].melBins;
					curHop = _models[_index].hop;
					try
					{
						if (!_models[_index].dicFolder.empty())
							PhonesPair = GetPhonesPairMap(_models[_index].dicFolder);
						else
							PhonesPair = GetPhonesPairMap(_path + L".json");
						Phones = GetPhones(PhonesPair);
					}
					catch (std::exception& _exception)
					{
						MessageBox(hWnd, (L"加载模型失败，字典文件不存在" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					}
					MidiVer = _models[_index].MidiVer;
				}
				else
				{
					MessageBox(hWnd, L"模型配置文件有误", L"错误！", MB_OK);
					modlist_main->SetCurSelItem(static_cast<int>(_models.size()) - 1);
					ClearModel();
					return true;
				}
				sampingRate = _models[_index].Samrate;
				n_speakers = static_cast<uint32_t>(_models[_index].Speakers.size());
				voice_imgname->SetAttribute(L"text", L"当前模型：" + _models[_index].Name);
				const auto bgimg = DMResources().ReadFiles(_path + L".png");
				if (bgimg.data)
					voice_imgbox->SetImage(bgimg);
				else
				{
					const auto defaultBg = DMResources().ReadFiles(GetCurrentFolder() + L"\\default.png");
					if(defaultBg.data)
						voice_imgbox->SetImage(defaultBg);
				}
				int64_t indexS = 0;
				for (auto it : _models[_index].Symbol)
					_symbol.insert(std::pair<wchar_t, int64_t>(it, indexS++));
				tts_begin->SetEnabled(true);
				if (!_models[_index].Cleaner.empty())
					switch (pluginApi.Load(_models[_index].Cleaner))
					{
					case (-1):
						MessageBox(hWnd, L"加载插件失败，可能是插件文件不存在\n", L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					case (1):
						MessageBox(hWnd, L"加载插件失败，可能是插件程序错误\n", L"失败", MB_OK);
						ClearModel();
						modlist_main->SetCurSelItem(-1);
						return true;
					default:
						clean_begin->SetEnabled(true);
						break;
					}
				return true;
			}
		);
		RegisterValEvent
		(
			L"modlist_child2",
			[&](uint64_t _index)
			{
				if (_index > TModelFolder.size() || _index < 0)
					_index = modlist_child2->GetCurSelItem();
				ClearTModel();
				if (_index == 0)
					return true;
				std::wstring symbols;
				const std::wstring _path = GetCurrentFolder() + L"\\Mods\\" + TModelFolder[_index - 1] + L"\\" + TModelFolder[_index - 1];
				try
				{
					sessionDec = new Ort::Session(*env, (_path + L"_dec.onnx").c_str(), *session_options);
					sessionDp = new Ort::Session(*env, (_path + L"_dp.onnx").c_str(), *session_options);
					sessionEnc_p = new Ort::Session(*env, (_path + L"_enc_p.onnx").c_str(), *session_options);
					sessionFlow = new Ort::Session(*env, (_path + L"_flow.onnx").c_str(), *session_options);
					if (_waccess((_path + L"_emb.onnx").c_str(), 0) != -1)
						sessionEmb = new Ort::Session(*env, (_path + L"_emb.onnx").c_str(), *session_options);
				}
				catch (Ort::Exception& _exception)
				{
					MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(_exception.what())).c_str(), L"失败", MB_OK);
					ClearTModel();
					modlist_child2->SetCurSelItem(0);
					return true;
				}
				for (const auto& it : _models)
					if (it.Folder == TModelFolder[_index - 1] && !it.Cleaner.empty())
					{
						tModelSr = it.Samrate;
						switch (pluginApi.Load(it.Cleaner))
						{
						case (-1):
							MessageBox(hWnd, L"加载插件失败，可能是插件文件不存在\n", L"失败", MB_OK);
							ClearTModel();
							modlist_child2->SetCurSelItem(0);
							return true;
						case (1):
							MessageBox(hWnd, L"加载插件失败，可能是插件程序错误\n", L"失败", MB_OK);
							ClearTModel();
							modlist_child2->SetCurSelItem(0);
							return true;
						default:
							clean_begin->SetEnabled(true);
							symbols = it.Symbol;
							emo = it.emo;
							try {
								if (emo)
								{
									const auto emopath = GetCurrentFolder() + L"\\emotion\\" + it.emoPath + L".npy";
									const auto emopathdef = GetCurrentFolder() + L"\\emotion\\" + it.emoPath + L".json";
									emoLoader.open(emopath);
									emoStringa.clear();
									EmoJson.Clear();
									std::ifstream EmoFiles(emopathdef.c_str());
									if (EmoFiles.is_open())
									{
										std::string JsonLine;
										while (std::getline(EmoFiles, JsonLine))
											emoStringa += JsonLine;
										EmoFiles.close();
										EmoJson.Parse(emoStringa.c_str());
									}
								}
							}
							catch (std::exception& e)
							{
								MessageBox(hWnd, (L"加载模型失败，可能是配置文件有误或模型有误" + to_wide_string(e.what())).c_str(), L"失败", MB_OK);
								ClearTModel();
								modlist_child2->SetCurSelItem(0);
								return true;
							}
							break;
						}
						break;
					}
				_symbol.clear();
				int64_t indes = 0;
				for (auto itC : symbols)
					_symbol.insert(std::pair<wchar_t, int64_t>(itC, indes++));
				length_scale_box->SetEnabled(true);
				noise_scale_box->SetEnabled(true);
				noise_scale_w_box->SetEnabled(true);
				tran_seeds->SetEnabled(true);
				enableTModel = true;
				return true;
			}
		);
	}

	void MioTTSControl::setConsDefalut() const
	{
		tran_seeds->SetEnabled(false);
		voice_imgname->SetAttribute(L"text", L"当前未选中模型");
		const auto defaultBg = DMResources().ReadFiles(GetCurrentFolder() + L"\\default.png");
		if (defaultBg.data)
			voice_imgbox->SetImage(defaultBg);
		tts_begin->SetEnabled(false);
		clean_begin->SetEnabled(false);
		voice_play->SetEnabled(false);
		voice_save->SetEnabled(false);
		voice_prog->SetEnabled(false);
		modlist_child->SetEnabled(false);
		length_scale_box->SetEnabled(false);
		noise_scale_box->SetEnabled(false);
		noise_scale_w_box->SetEnabled(false);
		modlist_child2->SetEnabled(false);
	}

	void MioTTSControl::initRegex()
	{
		std::wstring JsonPath = GetCurrentFolder() + L"\\ParamsRegex.json";
		std::string JsonData;
		std::ifstream ParamFiles(JsonPath.c_str());
		if (ParamFiles.is_open())
		{
			std::string JsonLine;
			while (std::getline(ParamFiles, JsonLine))
				JsonData += JsonLine;
			ParamFiles.close();
			rapidjson::Document ConfigJson;
			ConfigJson.Parse(JsonData.c_str());
			try
			{
				std::wstring NoiseScaleStr, NoiseScaleWStr, LengthScaleStr, SeedStr, CharaStr, BeginStr, emoStr;
				if (ConfigJson["NoiseScaleParam"].Empty())
					NoiseScaleStr = L"noise";
				else
					NoiseScaleStr = to_wide_string(ConfigJson["NoiseScaleParam"].GetString());

				if (ConfigJson["NoiseScaleWParam"].Empty())
					NoiseScaleWStr = L"noisew";
				else
					NoiseScaleWStr = to_wide_string(ConfigJson["NoiseScaleWParam"].GetString());

				if (ConfigJson["LengthScaleParam"].Empty())
					LengthScaleStr = L"length";
				else
					LengthScaleStr = to_wide_string(ConfigJson["LengthScaleParam"].GetString());

				if (ConfigJson["SeedParam"].Empty())
					SeedStr = L"seed";
				else
					SeedStr = to_wide_string(ConfigJson["SeedParam"].GetString());

				if (ConfigJson["CharaParam"].Empty())
					CharaStr = L"chara";
				else
					CharaStr = to_wide_string(ConfigJson["CharaParam"].GetString());

				if (ConfigJson["Begin"].Empty())
					BeginStr = L"<<";
				else
					BeginStr = to_wide_string(ConfigJson["Begin"].GetString());

				if (ConfigJson["End"].Empty())
					EndString = L">>";
				else
					EndString = to_wide_string(ConfigJson["End"].GetString());

				if (ConfigJson["EmoParam"].Empty())
					emoStr = L"emo";
				else
					emoStr = to_wide_string(ConfigJson["EmoParam"].GetString());
				NoiseScaleParam = BeginStr + L'(' + NoiseScaleStr + L":)([0-9\\.-]+)" + EndString;
				NoiseScaleWParam = BeginStr + L'(' + NoiseScaleWStr + L":)([0-9\\.-]+)" + EndString;
				LengthScaleParam = BeginStr + L'(' + LengthScaleStr + L":)([0-9\\.-]+)" + EndString;
				SeedParam = BeginStr + L'(' + SeedStr + L":)([0-9-]+)" + EndString;
				CharaParam = BeginStr + L'(' + CharaStr + L":)([0-9]+)" + EndString;
				EmoParam = BeginStr + L'(' + emoStr + L":)([^]+)" + EndString;
				EmoReg = L"[^" + BeginStr + EndString + L",;]+";
				if (EndString[0] == '\\')
					EndString = EndString.substr(1);
			}
			catch (std::exception& e)
			{
				MessageBoxA(hWnd, e.what(), R"(
				[Warn] Set Regex To Default (
				NoiseScaleParam = L"<<(noise:)([0-9\\.-]+)>>";
				NoiseScaleWParam = L"<<(noisew:)([0-9\\.-]+)>>";
				LengthScaleParam = L"<<(length:)([0-9\\.-]+)>>";
				SeedParam = L"<<(seed:)([0-9-]+)>>";
				CharaParam = L"<<(chara:)([0-9]+)>>";
				EndString = L">>";
				EmoParam = L"<<(emo:)(.)>>";)
				)", MB_ICONINFORMATION | MB_OK);
				NoiseScaleParam = L"<<(noise:)([0-9\\.-]+)>>";
				NoiseScaleWParam = L"<<(noisew:)([0-9\\.-]+)>>";
				LengthScaleParam = L"<<(length:)([0-9\\.-]+)>>";
				SeedParam = L"<<(seed:)([0-9-]+)>>";
				CharaParam = L"<<(chara:)([0-9]+)>>";
				EndString = L">>";
				EmoParam = L"<<(emo:)(.)>>";
			}
		}
		else
		{
			NoiseScaleParam = L"<<(noise:)([0-9\\.-]+)>>";
			NoiseScaleWParam = L"<<(noisew:)([0-9\\.-]+)>>";
			LengthScaleParam = L"<<(length:)([0-9\\.-]+)>>";
			SeedParam = L"<<(seed:)([0-9-]+)>>";
			CharaParam = L"<<(chara:)([0-9]+)>>";
			EmoParam = L"<<(emo:)(.)>>";
		}
	}

	void MioTTSControl::loadmodel()
	{
		ClearModel();
		std::wstring prefix = L"Cleaners";
		if (_waccess(prefix.c_str(), 0) == -1)
			if (_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created Cleaners Dir");
		prefix = L"emotion";
		if (_waccess(prefix.c_str(), 0) == -1)
			if(_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created emotion Dir");
		prefix = L"Mods";
		if (_waccess(prefix.c_str(), 0) == -1)
			if (_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created Mods Dir");
		prefix = L"temp";
		if (_waccess(prefix.c_str(), 0) == -1)
			if (_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created temp Dir");
		prefix = L"hifigan";
		if (_waccess(prefix.c_str(), 0) == -1)
			if (_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created hifigan Dir");
		prefix = L"hubert";
		if (_waccess(prefix.c_str(), 0) == -1)
			if (_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created hubert Dir");
		_wfinddata_t file_info;
		std::wstring current_path = GetCurrentFolder() + L"\\Mods";
		intptr_t handle = _wfindfirst((current_path + L"\\*.json").c_str(), &file_info);
		if (-1 == handle) {
			MessageBox(hWnd, L"未找到有效的模型定义文件", L"载入失败", MB_OK);
			return;
		}
		voice_imgname->SetAttribute(L"text", L"当前未选中模型");
		const auto defaultBg = DMResources().ReadFiles(GetCurrentFolder() + L"\\default.png");
		if (defaultBg.data)
			voice_imgbox->SetImage(defaultBg);
		if (modlist_main->GetCurSelItem() != -1)
			modlist_main->SetCurSelItem(-1);
		modlist_main->DeleteAllItem();
		_models.clear();
		do
		{
			std::string modInfo, modInfoAll;
			modelInfo _temp;
			std::ifstream modfile((current_path + L"\\" + file_info.name).c_str());
			while (std::getline(modfile, modInfo))
				modInfoAll += modInfo;
			modfile.close();
			rapidjson::Document modConfigJson;
			modConfigJson.Parse(modInfoAll.c_str());
			if (modConfigJson["Folder"].Empty() || modConfigJson["Name"].Empty() || modConfigJson["Type"].Empty() || modConfigJson["Rate"].IsNull())
				continue;
			_temp.Folder = to_wide_string(modConfigJson["Folder"].GetString());
			_temp.Name = to_wide_string(modConfigJson["Name"].GetString());
			_temp.Type = to_wide_string(modConfigJson["Type"].GetString());
			_temp.Samrate = modConfigJson["Rate"].GetInt();
			if (_temp.Type == L"Vits")
				_temp.Type = L"VITS_LJS";

			if (_temp.Type == L"VITS_VCTK" || _temp.Type == L"VITS_LJS" || _temp.Type == L"Tacotron2")
			{
				if (modConfigJson["Symbol"].Empty())
					continue;
				_temp.Symbol = to_wide_string(modConfigJson["Symbol"].GetString());
			}
			else if (_temp.Type != L"DiffSinger")
			{
				if (modConfigJson["Hop"].IsNull() || modConfigJson["Hubert"].Empty())
					continue;
				_temp.hop = modConfigJson["Hop"].GetInt();
				_temp.Hubert = to_wide_string(modConfigJson["Hubert"].GetString());
			}

			if (_temp.Type == L"Tacotron2" || _temp.Type == L"DiffSvc" || _temp.Type == L"DiffSinger")
			{
				if (modConfigJson["Hifigan"].Empty())
					continue;
				_temp.Hifigan = to_wide_string(modConfigJson["Hifigan"].GetString());
			}
			if ((_temp.Type == L"VITS_VCTK" || _temp.Type == L"SoVits") && modConfigJson["Characters"].Empty())
				continue;
			if (_temp.Type == L"DiffSvc")
			{
				if (modConfigJson["Pndm"].IsNull() || modConfigJson["MelBins"].IsNull())
					continue;
				_temp.pndm = modConfigJson["Pndm"].GetInt();
				_temp.melBins = modConfigJson["MelBins"].GetInt();
				if (!modConfigJson["V2"].IsNull())
					_temp.V2 = modConfigJson["V2"].GetBool();
			}
			if (_temp.Type == L"SoVits")
			{
				if (!modConfigJson["SoVits3"].IsNull())
					_temp.SV3 = modConfigJson["SoVits3"].GetBool();
				if (!modConfigJson["SoVits4"].IsNull())
					_temp.SV4 = modConfigJson["SoVits4"].GetBool();
			}
			if (!modConfigJson["Emotional"].IsNull())
				_temp.emo = modConfigJson["Emotional"].GetBool();
			if (_temp.emo)
			{
				if (modConfigJson["EmotionalPath"].Empty())
					continue;
				_temp.emoPath = to_wide_string(modConfigJson["EmotionalPath"].GetString());
			}
			if (_temp.Type == L"VITS_VCTK" || _temp.Type == L"VITS_LJS")
				_temp.Type = L"Vits";
			if (!modConfigJson["Cleaner"].Empty())
				_temp.Cleaner = to_wide_string(modConfigJson["Cleaner"].GetString());
			if (_temp.Type == L"DiffSinger")
			{
				if (modConfigJson["MelBins"].IsNull() || modConfigJson["Hop"].IsNull())
					continue;
				_temp.hop = modConfigJson["Hop"].GetInt();
				_temp.melBins = modConfigJson["MelBins"].GetInt();
				if (!modConfigJson["DicFolder"].Empty())
					_temp.dicFolder = to_wide_string(modConfigJson["DicFolder"].GetString());
				if (!modConfigJson["MidiVer"].IsNull())
					_temp.MidiVer = modConfigJson["MidiVer"].GetBool();
			}
			const auto charaList = modConfigJson["Characters"].GetArray();
			for (const auto& chara : charaList)
				_temp.Speakers.push_back(to_wide_string(chara.GetString()));
			auto _tmpItem = new ListItem;
			auto _tmpText = L"  " + _temp.Type + L":" + _temp.Name;
			if (_tmpText.size() > 30)
				_tmpText = _tmpText.substr(0, 30) + L"...";
			_tmpItem->SetText(_tmpText);
			modlist_main->AddItem(_tmpItem, -1);
			_models.push_back(_temp);
		} while (!_wfindnext(handle, &file_info));
		auto _tmpItem = new ListItem;
		_tmpItem->SetText(L"  未选中");
		modlist_main->AddItem(_tmpItem, -1);
	}

	void MioTTSControl::InitControl()
	{
		initRegex();
		const std::wstring prefix = L"OutPuts";
		if (_waccess(prefix.c_str(), 0) == -1)
			if (_wmkdir(prefix.c_str()))
				fprintf_s(stdout, "[Info] Created OutPuts Dir");
		RegisterControl();
		RegisterEvent();
		setConsDefalut();
		loadmodel();
		if(OleInitialize(nullptr) == S_OK)
		{
			const auto _lpdroptargetCallback = [&](const std::wstring& paths)
			{

				wave_editbox->SetCurText(paths);
			};
			const auto _lpdroptarget = new MDropTarget(_lpdroptargetCallback);
			RegisterDragDrop(hWnd, _lpdroptarget);
		}
	}

	void MioTTSControl::ClearModel()
	{
		emoStringa.clear();
		EmoJson.Clear();
		emo = false;
		emoLoader.close();

		_symbol.clear();

		modlist_child->SetCurSelItem(-1);
		modlist_child->DeleteAllItem();

		tran_seeds->SetEnabled(false);
		tts_begin->SetEnabled(false);
		clean_begin->SetEnabled(false);
		modlist_child->SetEnabled(false);
		length_scale_box->SetEnabled(false);
		noise_scale_box->SetEnabled(false);
		noise_scale_w_box->SetEnabled(false);

		pluginApi.unLoad();

		switch (_modelType)
		{
		case modelType::Vits:
		{
			delete sessionDec;
			delete sessionDp;
			delete sessionEnc_p;
			delete sessionFlow;
			delete sessionEmb;
			sessionDec = nullptr;
			sessionDp = nullptr;
			sessionEnc_p = nullptr;
			sessionFlow = nullptr;
			sessionEmb = nullptr;
			break;
		}
		case modelType::Taco:
		{
			delete sessionEncoder;
			delete sessionDecoderIter;
			delete sessionPostNet;
			delete sessionGan;
			sessionEncoder = nullptr;
			sessionDecoderIter = nullptr;
			sessionPostNet = nullptr;
			sessionGan = nullptr;
			break;
		}
		case modelType::diffSvc:
		{
			if (pred)
			{
				delete pred;
				delete denoise;
				delete after;
				delete encoder;
				pred = nullptr;
				denoise = nullptr;
				after = nullptr;
				encoder = nullptr;
			}
			else
			{
				delete diffSvc;
				diffSvc = nullptr;
			}
			delete nsfHifigan;
			nsfHifigan = nullptr;
			break;
		}
		case modelType::SoVits:
		{
			delete soVits;
			soVits = nullptr;
			break;
		}
		case modelType::diffSinger:
		{
			if (diffSinger)
			{
				delete diffSinger;
				diffSinger = nullptr;
			}
			else
			{
				delete pred;
				delete denoise;
				delete after;
				delete encoder;
				pred = nullptr;
				denoise = nullptr;
				after = nullptr;
				encoder = nullptr;
			}
			delete nsfHifigan;
			nsfHifigan = nullptr;
			break;
		}
		}

		//HiddenUnitBERTSessions
		if (hubert)
		{
			delete hubert;
			hubert = nullptr;
		}

		//TModel
		if (enableTModel)
		{
			delete sessionDec;
			delete sessionDp;
			delete sessionEnc_p;
			delete sessionFlow;
			delete sessionEmb;
			sessionDec = nullptr;
			sessionDp = nullptr;
			sessionEnc_p = nullptr;
			sessionFlow = nullptr;
			sessionEmb = nullptr;
			TModelFolder.clear();
			enableTModel = false;
		}

		//Env
		delete session_options;
		delete env;
		delete memory_info;
		env = nullptr;
		session_options = nullptr;
		memory_info = nullptr;

		PhonesPair.clear();
		Phones.clear();

		const auto defaultBg = DMResources().ReadFiles(GetCurrentFolder() + L"\\default.png");
		if (defaultBg.data)
			voice_imgbox->SetImage(defaultBg);
		else
			voice_imgbox->SetImage(nullptr);
#ifdef CUDAMOESS
		delete session_options_nsf;
		session_options_nsf = nullptr;
#endif
		modlist_child2->SetCurSelItem(0);
		modlist_child2->DeleteAllItem();
		modlist_child2->SetEnabled(false);
	}

	void MioTTSControl::InferQuit() const
	{
		modlist_refresh->SetEnabled(true);
		modlist_main->SetEnabled(true);
		tts_begin->SetEnabled(true);
		clean_begin->SetEnabled(true);
		process->SetVisible(false);
		process->SetAttribute(L"value", std::to_wstring(0));
	}

	void MioTTSControl::ClearTModel()
	{
		emoStringa.clear();
		EmoJson.Clear();
		emo = false;
		emoLoader.close();

		delete sessionDec;
		delete sessionDp;
		delete sessionEnc_p;
		delete sessionFlow;
		delete sessionEmb;
		sessionDec = nullptr;
		sessionDp = nullptr;
		sessionEnc_p = nullptr;
		sessionFlow = nullptr;
		sessionEmb = nullptr;
		enableTModel = false;

		pluginApi.unLoad();
		clean_begin->SetEnabled(false);
		noise_scale_box->SetEnabled(false);
		noise_scale_w_box->SetEnabled(false);
		length_scale_box->SetEnabled(false);
		_symbol.clear();
	}
}
