#include "MoeVSG2P.hpp"
#include "MJson.h"
#include "../../StringPreprocess.hpp"
#include <fstream>

MoeVoiceStudioG2PHeader

std::wregex SignRegex(L"[!@#$%^&*()_+\\-=`~,./;'\\[\\]<>?:\"{}|\\\\。？！，、；：“”‘’『』「」（）〔〕【】─…·—～《》〈〉]+");
std::wregex WordRegex(L"[^!@#$%^&*()_+\\-=`~,./;'\\[\\]<>?:\"{}|\\\\。？！，、；：“”‘’『』「」（）〔〕【】─…·—～《》〈〉]+");
std::wregex BlankRegex(L"[ ]+");
std::wregex ChineseRegex(L"^[\\u4e00-\\u9fa5]{0,}$");
std::wregex NumberRegex(L"\\d+(?:\\.?\\d+)?");
std::wstring ChineseNumber[] = { L"零",L"一",L"二",L"三",L"四",L"五",L"六",L"七",L"八",L"九",L"十" };
std::wstring ChineseNumberDigit[] = { L"",L"十",L"百",L"千",L"万",L"十万",L"百万",L"千万",L"亿" };
std::wstring JapaneseNumber[] = { L"零",L"一",L"ニ",L"三",L"四",L"五",L"六",L"七",L"八",L"九",L"十" };
std::wstring JapaneseNumberDigit[] = { L"",L"十",L"百",L"千",L"万",L"十万",L"百万",L"千万",L"億" };
std::unordered_map<std::wstring, std::wstring> _PUNCTUATION_MAP{
	{ L"：", L"," }, { L"；", L"," }, { L"，", L"," }, { L"。", L"." }, { L"！", L"!" }, { L"？", L"?" },
	{ L"·", L"," }, { L"、", L"," }, { L"...", L"…" }, { L"$", L"." }, { L"“", L"'" },
	{ L"”", L"'" }, { L"‘", L"'" }, { L"’", L"'" }, { L"（", L"'" }, { L"）", L"'" }, { L"(", L"'" },
	{ L")", L"'" }, { L"《", L"'" }, { L"》", L"'" }, { L"【", L"'" }, { L"】", L"'" }, { L"[", L"'" },
	{ L"]", L"'" }, { L"—", L"-" }, { L"～", L"-" }, { L"~", L"-" }, { L"「", L"'" }, { L"」", L"'" }
};
std::unordered_map<std::wstring, std::wstring> _ALPHASYMBOL_MAP{
	{L"#", L"シャープ"}, { L"%", L"パーセント" }, { L"&", L"アンド" }, { L"+", L"プラス" }, { L"-", L"マイナス" },
	{ L":", L"コロン" }, { L";", L"セミコロン" }, { L"<", L"小なり" }, { L"=", L"イコール" }, { L">", L"大なり" },
	{ L"@", L"アット" }, { L"a", L"エー" }, { L"b", L"ビー" }, { L"c", L"シー" }, { L"d", L"ディー" }, { L"e", L"イー" },
	{ L"f", L"エフ" }, { L"g", L"ジー" }, { L"h", L"エイチ" }, { L"i", L"アイ" }, { L"j", L"ジェー" }, { L"k", L"ケー" },
	{ L"l", L"エル" }, { L"m", L"エム" }, { L"n", L"エヌ" }, { L"o", L"オー" }, { L"p", L"ピー" }, { L"q", L"キュー" },
	{ L"r", L"アール" }, { L"s", L"エス" }, { L"t", L"ティー" }, { L"u", L"ユー" }, { L"v", L"ブイ" }, { L"w", L"ダブリュー" },
	{ L"x", L"エックス" }, { L"y", L"ワイ" }, { L"z", L"ゼット" }, { L"α", L"アルファ" }, { L"β", L"ベータ" }, { L"γ", L"ガンマ" },
	{ L"δ", L"デルタ" }, { L"ε", L"イプシロン" }, { L"ζ", L"ゼータ" }, { L"η", L"イータ" }, { L"θ", L"シータ" }, { L"ι", L"イオタ" },
	{ L"κ", L"カッパ" }, { L"λ", L"ラムダ" }, { L"μ", L"ミュー" }, { L"ν", L"ニュー" }, { L"ξ", L"クサイ" }, { L"ο", L"オミクロン" },
	{ L"π", L"パイ" }, { L"ρ", L"ロー" }, { L"σ", L"シグマ" }, { L"τ", L"タウ" }, { L"υ", L"ウプシロン" }, { L"φ", L"ファイ" },
	{ L"χ", L"カイ" }, { L"ψ", L"プサイ" }, { L"ω", L"オメガ", }};
std::vector<std::pair<std::wstring, std::wstring>> _CURRENCY_MAP{{L"\\$", L"ドル"}, { L"¥", L"円" }, { L"£", L"ポンド" }, { L"€", L"ユーロ" }};

MVSCleaner DefaultCleaner;

MVSCleaner* GetDefCleaner()
{
	return &DefaultCleaner;
}

#ifdef WIN32
MoeVoiceStudioG2PApi::~MoeVoiceStudioG2PApi()
{
	unLoad();
}

MoeVoiceStudioG2PApi& MoeVoiceStudioG2PApi::operator=(MoeVoiceStudioG2PApi&& move) noexcept
{
	func = move.func;
	m_hDynLib = move.m_hDynLib;
	move.func = nullptr;
	move.m_hDynLib = nullptr;
	return *this;
}

bool MoeVoiceStudioG2PApi::enabled() const
{
	return m_hDynLib != nullptr;
}

MoeVoiceStudioG2PApi::SplitData MoeVoiceStudioG2PApi::GetSplitWords(const std::wstring& inputLen) const
{
	SplitData TempData;
	if (getvocab)
		TempData = (*(SplitData*)getvocab(inputLen.c_str()));
	return TempData;
}

void MoeVoiceStudioG2PApi::LoadDict(const std::wstring& Path) const
{
	if (loaddic)
		loaddic(Path.c_str());
}

char MoeVoiceStudioG2PApi::Load(const std::wstring& PluginName)
{
	func = nullptr;
	frel = nullptr;
	if (m_hDynLib)
	{
		FreeLibrary(m_hDynLib);
		m_hDynLib = nullptr;
	}
	m_hDynLib = LoadLibrary((PluginName).c_str());
	if (m_hDynLib == nullptr)
		return -1;
	func = reinterpret_cast<funTy>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "PluginMain")
			)
		);
	frel = reinterpret_cast<freTy>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "Release")
			)
		);
	getvocab = reinterpret_cast<vocabFn>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "GetSplitData")
			)
		);
	vocabrel = reinterpret_cast<freTy>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "RefreshTokenizer")
			)
		);
	loaddic = reinterpret_cast<loadFn>(
		reinterpret_cast<void*>(
			GetProcAddress(m_hDynLib, "LoadDict")
			)
		);
	if (func == nullptr)
		return 1;
	return 0;
}

std::wstring MoeVoiceStudioG2PApi::functionAPI(const std::wstring& inputLen, const std::wstring& placeholderSymbol,
	const std::wstring& extraInfo, int64_t languageID) const
{
	if (func)
	{
		const auto tmp = func(inputLen.c_str(), placeholderSymbol.c_str(), extraInfo.c_str(), languageID);
		std::wstring ret = tmp;
		return ret;
	}
	return inputLen;
}

void MoeVoiceStudioG2PApi::unLoad()
{
	if (frel)
		frel();
	if (vocabrel)
		vocabrel();
	vocabrel = nullptr;
	getvocab = nullptr;
	loaddic = nullptr;
	func = nullptr;
	frel = nullptr;
	if (m_hDynLib)
		FreeLibrary(m_hDynLib);
	m_hDynLib = nullptr;
}
#endif

void MVSDict::GetDict(const std::wstring& path)
{
	PlaceholderSymbol = L"|";
	std::string phoneInfo, phoneInfoAll;
	std::ifstream phonefile(path.c_str());
	if (!phonefile.is_open())
		throw std::exception("phone file not found");
	while (std::getline(phonefile, phoneInfo))
		phoneInfoAll += phoneInfo;
	phonefile.close();
	MJson PhoneJson;
	PhoneJson.Parse(phoneInfoAll);
	if (PhoneJson.HasParseError())
		throw std::exception("json file error");
	for (const auto& itr : PhoneJson.GetMemberArray())
	{
		std::wstring Key = to_wide_string(itr.first);
		if (Key == L"PlaceholderSymbol")
		{
			if (itr.second.IsString() && itr.second.GetStringLength())
				PlaceholderSymbol = to_wide_string(itr.second.GetString());
			if (PlaceholderSymbol.length() > 1)
				PlaceholderSymbol = L"|";
			continue;
		}
		const auto Value = itr.second.GetArray();
		_Dict[Key] = std::vector<std::wstring>();
		for (const auto& it : Value)
			_Dict[Key].push_back(to_wide_string(it.GetString()));
	}
}

std::vector<std::wstring> MVSDict::DictReplace(const std::vector<std::wstring>& input) const
{
	std::vector<std::wstring> _out;
	for (const auto& i : input)
		if (_Dict.find(i) != _Dict.end())
		{
			const auto& Value = _Dict.at(i);
			_out.insert(_out.end(), Value.begin(), Value.end());
		}
		else
			_out.emplace_back(i);
	return _out;
}

std::vector<std::wstring> MVSDict::DictReplace(const std::wstring& input, const std::wstring& tPlaceholderSymbol) const
{
	std::vector<std::wstring> _output;
	auto tmp = input;
	tmp += tPlaceholderSymbol;
	while (!tmp.empty())
	{
		const size_t pos = tmp.find(tPlaceholderSymbol);
		const auto Key = tmp.substr(0, pos);
		tmp = tmp.substr(pos + 1);
		if (_Dict.find(Key) != _Dict.end())
		{
			const auto& Value = _Dict.at(Key);
			_output.insert(_output.end(), Value.begin(), Value.end());
		}
		else
			_output.emplace_back(Key);
	}
	return _output;
}

std::wstring MVSDict::DictReplaceGetStr(const std::wstring& input, const std::wstring& tPlaceholderSymbol, bool usePlaceholderSymbol) const
{
	const auto tmp = DictReplace(input, tPlaceholderSymbol);
	std::wstring output;
	for (const auto& i : tmp)
		if (usePlaceholderSymbol)
			output += i + tPlaceholderSymbol;
		else
			output += i;
	return output;
}

void Tokenizer::load(const std::wstring& _Path)
{
	const MJson _VocabJson(to_byte_string(_Path).c_str());
	if (!_VocabJson.HasMember("ContinuingSubwordPrefix") ||
		!_VocabJson.HasMember("Type") ||
		!_VocabJson.HasMember("Vocab") ||
		_VocabJson["ContinuingSubwordPrefix"].Empty() ||
		_VocabJson["Type"].Empty() ||
		!_VocabJson["ContinuingSubwordPrefix"].IsString() ||
		!_VocabJson["Type"].IsString())
		throw std::exception("Vocab.json Error");
	const std::string Type = _VocabJson["Type"].GetString();
	if (Type == "Unigram") Model = TokenizerModel::Unigram;
	Symbol = to_wide_string(_VocabJson["ContinuingSubwordPrefix"].GetString());

	if(Model == TokenizerModel::WordPiece)
	{
		if(_VocabJson["Vocab"].IsArray())
		{
			const auto _VocabArray = _VocabJson["Vocab"].GetArray();
			int64_t Index = 0;
			for (const auto& Object : _VocabArray)
				Vocab[to_wide_string(Object.GetString())] = Index++;
		}
		else
		{
			const auto _VocabDict = _VocabJson["Vocab"].GetMemberArray();
			for (const auto& Pair : _VocabDict)
			{
				if (Pair.second.IsInt())
					Vocab[to_wide_string(Pair.first)] = TokenizerType(Pair.second.GetInt());
				else if (Pair.second.IsFloat())
					Vocab[to_wide_string(Pair.first)] = TokenizerType(Pair.second.GetFloat());
			}
		}
	}
	else
	{
		const auto _VocabArray = _VocabJson["Vocab"].GetArray();
		int64_t Index = 0;
		for (const auto& Object : _VocabArray)
			Vocab[to_wide_string(Object.GetArray()[0].GetString())] = Index++;
	}
	if (_VocabJson.HasMember("UseSplit") && _VocabJson["UseSplit"].IsBool())
		UseSplit = _VocabJson["UseSplit"].GetBool();
}

void Tokenizer::loadCleaner(const std::wstring& _Path) const
{
	if (Cleaner)
		Cleaner->loadG2p(_Path);
}

void Tokenizer::loadDict(const std::wstring& _Path) const
{
	if (Cleaner)
		Cleaner->loadDict(_Path);
}

std::vector<Tokenizer::TokenizerType> Tokenizer::UnigramMethod(const std::wstring& Seq, size_t MaxWordLength, TokenizerMethod Method) const
{
	if (Seq.empty())
		return {};
	//auto SeqVector = SplitString(Seq, SignRegex);
	std::vector<TokenizerType> Tokens;
	Tokens.emplace_back(Vocab.at(L"[CLS]"));
	const auto UNKId = Vocab.at(L"[UNK]");
	std::wstring SeqWord = Seq;
	if (Method == TokenizerMethod::Left)
	{
		bool FirstTime = true;
		while (!SeqWord.empty())
		{
			for (size_t SearchLength = min(MaxWordLength, SeqWord.length()); SearchLength > 0; --SearchLength)
			{
				if (FirstTime)
				{
					size_t SubVal = 0;
					if (SearchLength > Symbol.length())
						SubVal = Symbol.length();
					const auto SearchResult = Vocab.find(Symbol + SeqWord.substr(0, SearchLength - SubVal));
					if (SearchResult != Vocab.end())
					{
						Tokens.emplace_back(SearchResult->second);
						SeqWord = SeqWord.substr(SearchLength - SubVal);
						FirstTime = false;
						break;
					}
				}
				const auto SearchResult = Vocab.find(SeqWord.substr(0, SearchLength));
				if (SearchResult != Vocab.end())
				{
					Tokens.emplace_back(SearchResult->second);
					SeqWord = SeqWord.substr(SearchLength);
					if (FirstTime) FirstTime = false;
					break;
				}
				if (SearchLength == 1)
				{
					const auto SubStr = SeqWord.substr(0, SearchLength);
					const auto SearchRes = _PUNCTUATION_MAP.find(SubStr);
					if (SearchRes != _PUNCTUATION_MAP.end())
					{
						const auto SearchR = Vocab.find(SearchRes->second);
						if (SearchR != Vocab.end())
							Tokens.emplace_back(SearchR->second);
						SeqWord = SeqWord.substr(1);
						break;
					}
					if (Tokens.empty() || Tokens.back() != UNKId)
						Tokens.emplace_back(UNKId);
					SeqWord = SeqWord.substr(1);
				}
			}
		}
	}
	else
		throw std::exception("NotImplementedError");
	Tokens.emplace_back(Vocab.at(L"[SEP]"));
	return Tokens;
}

std::vector<Tokenizer::TokenizerType> Tokenizer::WordPieceMethod(const std::wstring& Seq, size_t MaxWordLength, TokenizerMethod Method) const
{
	if (Seq.empty())
		return {};
	auto SeqVector = SplitString(Seq, SignRegex);
	std::vector<TokenizerType> Tokens;
	Tokens.emplace_back(Vocab.at(L"[CLS]"));
	const auto UNKId = Vocab.at(L"[UNK]");
	if (Method == TokenizerMethod::Left)
	{
		for (auto& SeqWord : SeqVector)
		{
			bool FirstTime = true;
			while (!SeqWord.empty())
			{
				if (regex_match(SeqWord.substr(0, 1), ChineseRegex))
				{
					const auto SearchResult = Vocab.find(SeqWord.substr(0, 1));
					if (SearchResult != Vocab.end())
						Tokens.emplace_back(SearchResult->second);
					else
						Tokens.emplace_back(UNKId);
					SeqWord = SeqWord.substr(1);
					continue;
				}
				for (size_t SearchLength = min(MaxWordLength, SeqWord.length()); SearchLength > 0; --SearchLength)
				{
					if (!FirstTime)
					{
						size_t SubVal = 0;
						if (SearchLength > Symbol.length())
							SubVal = Symbol.length();
						const auto SearchResult = Vocab.find(Symbol + SeqWord.substr(0, SearchLength - SubVal));
						if (SearchResult != Vocab.end())
						{
							Tokens.emplace_back(SearchResult->second);
							SeqWord = SeqWord.substr(SearchLength - SubVal);
							break;
						}
					}
					const auto SearchResult = Vocab.find(SeqWord.substr(0, SearchLength));
					if (SearchResult != Vocab.end())
					{
						Tokens.emplace_back(SearchResult->second);
						SeqWord = SeqWord.substr(SearchLength);
						if (FirstTime) FirstTime = false;
						break;
					}
					if (SearchLength == 1)
					{
						const auto SubStr = SeqWord.substr(0, SearchLength);
						const auto SearchRes = _PUNCTUATION_MAP.find(SubStr);
						if (SearchRes != _PUNCTUATION_MAP.end())
						{
							const auto SearchR = Vocab.find(SearchRes->second);
							if (SearchR != Vocab.end())
								Tokens.emplace_back(SearchR->second);
							SeqWord = SeqWord.substr(1);
							break;
						}
						if (Tokens.empty() || Tokens.back() != UNKId)
							Tokens.emplace_back(UNKId);
						SeqWord = SeqWord.substr(1);
					}
				}
			}
		}
	}
	else
		throw std::exception("NotImplementedError");
	Tokens.emplace_back(Vocab.at(L"[SEP]"));
	return Tokens;
}

std::vector<Tokenizer::TokenizerType> Tokenizer::operator()(const std::wstring& Seq, size_t MaxWordLength, TokenizerMethod Method) const
{
	if (Model == TokenizerModel::WordPiece)
		return WordPieceMethod(Seq, MaxWordLength, Method);
	return UnigramMethod(Seq, MaxWordLength, Method);
}

std::vector<std::wstring> Tokenizer::SplitString(const std::wstring& _InputRef, const std::wregex & _SignRegex)
{
	if (_InputRef.empty())
		return {};
	std::wstring InputStr = _InputRef;
	std::vector<std::wstring> TmpStrVec, StrVec;
	std::wsmatch MatchedSign;
	while (std::regex_search(InputStr, MatchedSign, _SignRegex))
	{
		if (MatchedSign.prefix().matched)
			TmpStrVec.push_back(MatchedSign.prefix());
		TmpStrVec.push_back(MatchedSign.str());
		InputStr = MatchedSign.suffix();
	}
	if (!InputStr.empty())
		TmpStrVec.emplace_back(InputStr);
	for(const auto& i : TmpStrVec)
	{
		std::wsregex_token_iterator TokenIter(i.begin(), i.end(), BlankRegex, -1);
		decltype(TokenIter) TokenIterEnd;
		for (; TokenIter != TokenIterEnd; ++TokenIter)
			if (!TokenIter->str().empty())
				StrVec.push_back(TokenIter->str());
	}
	return StrVec;
}

std::vector<std::wstring> Tokenizer::SplitWithPlugin(const std::vector<std::wstring>& _Inputs) const
{
	std::vector<std::wstring> SeqVec;
	for(const auto& Seq : _Inputs)
	{
		const auto SplitedWords = GetCleaner().GetCleaner().GetSplitWords(Seq);
		for (size_t i = 0; i < SplitedWords.Size; ++i)
		{
			auto TmpString = to_wide_string(SplitedWords.Data[i]);
			TmpString = TmpString.substr(0, TmpString.find(L','));
			SeqVec.emplace_back(std::move(TmpString));
		}
	}
	return SeqVec;
}

std::wstring NumberToChinese(double Number)
{
	std::wstring StrRtn;
	std::wstring InputStr = std::to_wstring(Number);
	const size_t PIndex = InputStr.find(L'.');
	std::wstring IntegerStr, FractionStr;
	if (PIndex != std::wstring::npos)
	{
		IntegerStr = InputStr.substr(0, PIndex);
		FractionStr = InputStr.substr(PIndex + 1);
		while (!FractionStr.empty() && FractionStr.back() == L'0')
			FractionStr.pop_back();
	}
	else
		IntegerStr = std::move(InputStr);

	if (IntegerStr != L"0")
	{
		size_t MaxIntegerStrLength = IntegerStr.length();
		for (; MaxIntegerStrLength > 0; --MaxIntegerStrLength)
			if (IntegerStr[MaxIntegerStrLength - 1] != L'0')
				break;
		if (MaxIntegerStrLength < 1)
			MaxIntegerStrLength = 1;

		const auto DigitNum = IntegerStr.length();
		for (size_t i = 0; i < MaxIntegerStrLength; i++)
		{
			const auto NumberIndex = IntegerStr[i] - L'0';
			const auto DigitIndex = DigitNum - i - 1;
			if (0 == NumberIndex)
			{
				if ((i > 0 && L'0' == IntegerStr[i - 1]) || i == IntegerStr.length() - 1)
					continue;
				if (DigitIndex >= 4 && 0 == DigitIndex % 4)
					StrRtn += ChineseNumberDigit[DigitIndex];
				else
					StrRtn += ChineseNumber[NumberIndex];
			}
			else
			{
				StrRtn += ChineseNumber[NumberIndex];
				if (IntegerStr.length() == 2 && IntegerStr[0] == '1' && i == 0)
					StrRtn.erase(0);
				if (0 == DigitIndex % 4)
					StrRtn += ChineseNumberDigit[DigitIndex];
				else
					StrRtn += ChineseNumberDigit[DigitIndex % 4];
			}
		}
	}
	else
		StrRtn += L"零";

	if (!FractionStr.empty())
		StrRtn += L"点";
	for(const auto FractionI : FractionStr)
	{
		const auto NumberIndex = FractionI - L'0';
		StrRtn += ChineseNumber[NumberIndex];
	}
	return StrRtn;
}

std::wstring NumberToJapanese(double Number)
{
	std::wstring StrRtn;
	std::wstring InputStr = std::to_wstring(Number);
	const size_t PIndex = InputStr.find(L'.');
	std::wstring IntegerStr, FractionStr;
	if (PIndex != std::wstring::npos)
	{
		IntegerStr = InputStr.substr(0, PIndex);
		FractionStr = InputStr.substr(PIndex + 1);
		while (!FractionStr.empty() && FractionStr.back() == L'0')
			FractionStr.pop_back();
	}
	else
		IntegerStr = std::move(InputStr);

	if (IntegerStr != L"0")
	{
		size_t MaxIntegerStrLength = IntegerStr.length();
		for (; MaxIntegerStrLength > 0; --MaxIntegerStrLength)
			if (IntegerStr[MaxIntegerStrLength - 1] != L'0')
				break;
		if (MaxIntegerStrLength < 1)
			MaxIntegerStrLength = 1;

		const auto DigitNum = IntegerStr.length();
		for (size_t i = 0; i < MaxIntegerStrLength; i++)
		{
			const auto NumberIndex = IntegerStr[i] - L'0';
			const auto DigitIndex = DigitNum - i - 1;
			if (0 == NumberIndex)
			{
				if ((i > 0 && L'0' == IntegerStr[i - 1]) || i == IntegerStr.length() - 1)
					continue;
				if (DigitIndex >= 4 && 0 == DigitIndex % 4)
					StrRtn += JapaneseNumberDigit[DigitIndex];
				else
					StrRtn += JapaneseNumber[NumberIndex];
			}
			else
			{
				StrRtn += JapaneseNumber[NumberIndex];
				if (IntegerStr.length() == 2 && IntegerStr[0] == '1' && i == 0)
					StrRtn.erase(0);
				if (0 == DigitIndex % 4)
					StrRtn += JapaneseNumberDigit[DigitIndex];
				else
					StrRtn += JapaneseNumberDigit[DigitIndex % 4];
			}
		}
	}
	else
		StrRtn += L"零";

	if (!FractionStr.empty())
		StrRtn += L"点";
	for (const auto FractionI : FractionStr)
	{
		const auto NumberIndex = FractionI - L'0';
		StrRtn += JapaneseNumber[NumberIndex];
	}
	return StrRtn;
}

std::wstring ChineseNormalize(const std::wstring& _Input)
{
	std::wstring RtnStr;
	const auto StrVec = Tokenizer::SplitString(_Input, NumberRegex);
	for(const auto& Str : StrVec)
	{
		if (std::regex_match(Str, NumberRegex))
			RtnStr += NumberToChinese(_wtof(Str.c_str()));
		else
			RtnStr += Str;
	}
	RtnStr = std::regex_replace(RtnStr, std::wregex(L"嗯"), L"恩");
	RtnStr = std::regex_replace(RtnStr, std::wregex(L"呣"), L"母");
	return RtnStr;
}

std::wstring JapaneseNormalize(const std::wstring& _Input)
{
	std::wstring RtnStr;
	const auto StrVec = Tokenizer::SplitString(_Input, NumberRegex);
	for (const auto& Str : StrVec)
	{
		if (std::regex_match(Str, NumberRegex))
			RtnStr += NumberToJapanese(_wtof(Str.c_str()));
		else
			RtnStr += Str;
	}
	for (const auto& PunPair : _CURRENCY_MAP)
		RtnStr = std::regex_replace(RtnStr, std::wregex(PunPair.first), PunPair.second);
	return RtnStr;
}

std::wstring NormalizeText(const std::wstring& _Input, const std::string& _Language)
{
	if (_Language == "ZH")
		return ChineseNormalize(_Input);
	if (_Language == "JP")
		return JapaneseNormalize(_Input);
	return _Input;
}

MoeVoiceStudioG2PEnd