/**
 * FileName: MoeVSG2P.hpp
 * Note: MoeVoiceStudioCore G2P及字典（TTS用）
 *
 * Copyright (C) 2022-2023 NaruseMioShirakana (shirakanamio@foxmail.com)
 *
 * This file is part of MoeVoiceStudioCore library.
 * MoeVoiceStudioCore library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Affero General Public License as published by the Free Software Foundation, either version 3
 * of the License, or any later version.
 *
 * MoeVoiceStudioCore library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with Foobar.
 * If not, see <https://www.gnu.org/licenses/agpl-3.0.html>.
 *
 * date: 2023-11-9 Create
*/

#pragma once
#include <string>
#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#include <Windows.h>
#endif
#include <map>
#include <regex>
#include <vector>
#include <unordered_map>

#define MoeVoiceStudioG2PHeader namespace MoeVSG2P {
#define MoeVoiceStudioG2PEnd }

MoeVoiceStudioG2PHeader

class MoeVoiceStudioG2PApi
{
public:
	struct SplitData
	{
		char** Data = nullptr;
		size_t Size = 0;
	};
	using funTy = const wchar_t* (*)(const wchar_t*, const wchar_t*, const wchar_t*, const char*);
	using freTy = void (*)();
	using vocabFn = void* (*)(const wchar_t*);
	using loadFn = void (*)(const wchar_t*);
	MoeVoiceStudioG2PApi() = default;
	~MoeVoiceStudioG2PApi();
	char Load(const std::wstring& PluginName);
	void unLoad();
	void ReleaseVoc() const
	{
		if (vocabrel)
			vocabrel();
	}
	[[nodiscard]] SplitData GetSplitWords(const std::wstring& inputLen) const;
	[[nodiscard]] std::wstring functionAPI(const std::wstring& inputLen, const std::wstring& placeholderSymbol,
		const std::wstring& extraInfo, const std::string& languageID) const;
	MoeVoiceStudioG2PApi(const MoeVoiceStudioG2PApi&) = delete;
	MoeVoiceStudioG2PApi(MoeVoiceStudioG2PApi&&) = delete;
	MoeVoiceStudioG2PApi& operator=(MoeVoiceStudioG2PApi&& move) noexcept;
	[[nodiscard]] bool enabled() const;
	MoeVoiceStudioG2PApi& operator=(const MoeVoiceStudioG2PApi&) = delete;
	void LoadDict(const std::wstring& Path) const;
private:
#ifdef WIN32
	const wchar_t*(*func)(const wchar_t*, const wchar_t*, const wchar_t*, const char*) = nullptr;
	void (*frel)() = nullptr;
	void* (*getvocab)(const wchar_t*) = nullptr;
	void (*vocabrel)() = nullptr;
	void (*loaddic)(const wchar_t*) = nullptr;
	HINSTANCE m_hDynLib = nullptr;
#endif
};

class MVSDict
{
public:
	MVSDict() = default;
	~MVSDict() = default;

	[[nodiscard]] bool enabled() const
	{
		return !_Dict.empty();
	}

	void unload()
	{
		_Dict.clear();
	}

	[[nodiscard]] std::vector<std::wstring> DictReplace(const std::vector<std::wstring>& input) const;

	[[nodiscard]] std::vector<std::wstring> DictReplace(const std::wstring& input, const std::wstring& tPlaceholderSymbol) const;

	[[nodiscard]] std::wstring DictReplaceGetStr(const std::wstring& input, const std::wstring& tPlaceholderSymbol, bool usePlaceholderSymbol = true) const;

	void GetDict(const std::wstring& path);

	[[nodiscard]] std::wstring getPlaceholderSymbol() const
	{
		return PlaceholderSymbol;
	}
private:
	std::map<std::wstring, std::vector<std::wstring>> _Dict;
	std::wstring PlaceholderSymbol = L"|";
};

class MVSCleaner
{
public:
	MVSCleaner() = default;

	~MVSCleaner()
	{
		unloadDict();
		unloadG2p();
	}

	void unloadDict()
	{
		_Dict.unload();
	}

	void unloadG2p()
	{
		_G2p.unLoad();
	}

	void loadDict(const std::wstring& _path)
	{
		if (_Dict.enabled())
			unloadDict();
		_Dict.GetDict(_path);
	}

	void loadG2p(const std::wstring& _path)
	{
		if (_G2p.enabled())
			unloadG2p();
		_G2p.Load(_path);
	}

	[[nodiscard]] bool G2pEnabled() const
	{
		return _G2p.enabled();
	}

	[[nodiscard]] bool DictEnabled() const
	{
		return _Dict.enabled();
	}

	[[nodiscard]] std::wstring G2p(const std::wstring& _text, const std::wstring& placeholderSymbol,
		const std::wstring& extraInfo, std::string languageID) const
	{
		return _G2p.functionAPI(_text, placeholderSymbol, extraInfo, languageID);
	}

	[[nodiscard]] const MoeVoiceStudioG2PApi& GetCleaner() const
	{
		return _G2p;
	}

	[[nodiscard]] auto DictReplace(const std::vector<std::wstring>& input) const
	{
		return _Dict.DictReplace(input);
	}

	[[nodiscard]] auto DictReplace(const std::wstring& input, const std::wstring& tPlaceholderSymbol) const
	{
		return _Dict.DictReplace(input, tPlaceholderSymbol);
	}

	[[nodiscard]] auto DictReplaceGetStr(const std::wstring& input, const std::wstring& tPlaceholderSymbol, bool usePlaceholderSymbol = true) const
	{
		return _Dict.DictReplaceGetStr(input, tPlaceholderSymbol, usePlaceholderSymbol);
	}

	[[nodiscard]] std::wstring getPlaceholderSymbol() const
	{
		return _Dict.getPlaceholderSymbol();
	}

protected:
	MoeVoiceStudioG2PApi _G2p;
	MVSDict _Dict;
};

class Tokenizer
{
public:
	using TokenizerType = int64_t;
	enum class TokenizerMethod
	{
		Left,
		Right
	};
	enum class TokenizerModel
	{
		Unigram,
		WordPiece
	};
	Tokenizer() = default;
	Tokenizer(const std::wstring& _Path)
	{
		load(_Path);
	}
	void BondCleaner(MVSCleaner* MCleaner)
	{
		Cleaner = MCleaner;
	}
	void load(const std::wstring& _Path);
	void loadCleaner(const std::wstring& _Path) const;
	void loadDict(const std::wstring& _Path) const;
	[[nodiscard]] const MVSCleaner& GetCleaner() const
	{
		return *Cleaner;
	}
	const MVSCleaner* operator->() const
	{
		return Cleaner;
	}
	[[nodiscard]] std::vector<std::wstring> WordPieceMethod(const std::wstring& Seq, size_t MaxWordLength = 25, TokenizerMethod Method = TokenizerMethod::Left) const;
	[[nodiscard]] std::vector<std::wstring> UnigramMethod(const std::wstring& Seq, size_t MaxWordLength = 25, TokenizerMethod Method = TokenizerMethod::Left) const;
	std::vector<TokenizerType> operator()(const std::vector<std::wstring>& Seq, bool SkipBlank = false) const;
	[[nodiscard]] std::vector<std::wstring> Tokenize(const std::wstring& Seq, size_t MaxWordLength = 25, TokenizerMethod Method = TokenizerMethod::Left) const;
	[[nodiscard]] std::vector<std::wstring> SplitWithPlugin(const std::vector<std::wstring>& _Inputs) const;
	static std::vector<std::wstring> SplitString(const std::wstring& _InputRef, const std::wregex& _SignRegex);
private:
	std::unordered_map<std::wstring, TokenizerType> Vocab;
	std::wstring Symbol = L"##";
	TokenizerModel Model = TokenizerModel::WordPiece;
	MVSCleaner* Cleaner = nullptr;
	bool UseSplit = false;
};

MVSCleaner* GetDefCleaner();

std::wstring JapaneseNormalize(const std::wstring& _Input);

std::wstring ChineseNormalize(const std::wstring& _Input);

std::wstring NormalizeText(const std::wstring& _Input, const std::string& _Language);

std::wstring NumberToChinese(double Number);

std::wstring NumberToJapanese(double Number);

MoeVoiceStudioG2PEnd