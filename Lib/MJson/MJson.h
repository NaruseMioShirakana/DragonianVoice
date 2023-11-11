#pragma once
#include "yyjson.h"
#include <vector>
class MJsonValue
{
public:
	MJsonValue() = default;
	MJsonValue(yyjson_val* _val) : _Ptr(_val) {}
	~MJsonValue() = default;
	MJsonValue(MJsonValue&& _val) noexcept
	{
		_Ptr = _val._Ptr;
		_val._Ptr = nullptr;
	}
	MJsonValue(const MJsonValue& _val) = delete;
	MJsonValue& operator=(MJsonValue&& _val) noexcept
	{
		_Ptr = _val._Ptr;
		_val._Ptr = nullptr;
		return *this;
	}
	MJsonValue& operator=(const MJsonValue& _val) = delete;
	[[nodiscard]] bool IsNull() const
	{
		return yyjson_is_null(_Ptr);
	}
	[[nodiscard]] bool IsBoolean() const
	{
		return yyjson_is_bool(_Ptr);
	}
	[[nodiscard]] bool IsBool() const
	{
		return yyjson_is_bool(_Ptr);
	}
	[[nodiscard]] bool IsInt() const
	{
		return yyjson_is_num(_Ptr);
	}
	[[nodiscard]] bool IsFloat() const
	{
		return yyjson_is_num(_Ptr);
	}
	[[nodiscard]] bool IsInt64() const
	{
		return yyjson_is_num(_Ptr);
	}
	[[nodiscard]] bool IsDouble() const
	{
		return yyjson_is_num(_Ptr);
	}
	[[nodiscard]] bool IsString() const
	{
		return yyjson_is_str(_Ptr);
	}
	[[nodiscard]] bool IsArray() const
	{
		return yyjson_is_arr(_Ptr);
	}
	[[nodiscard]] bool GetBool() const
	{
		return yyjson_get_bool(_Ptr);
	}
	[[nodiscard]] bool GetBoolean() const
	{
		return yyjson_get_bool(_Ptr);
	}
	[[nodiscard]] int GetInt() const
	{
		return yyjson_get_int(_Ptr);
	}
	[[nodiscard]] int64_t GetInt64() const
	{
		return yyjson_get_sint(_Ptr);
	}
	[[nodiscard]] float GetFloat() const
	{
		return float(yyjson_get_real(_Ptr));
	}
	[[nodiscard]] double GetDouble() const 
	{
		return yyjson_get_real(_Ptr);
	}
	[[nodiscard]] std::string GetString() const
	{
		if (const auto _str = yyjson_get_str(_Ptr))
			return _str;
		return "";
	}
	[[nodiscard]] std::vector<MJsonValue> GetArray() const
	{
		std::vector<MJsonValue> _ret;
		if (!IsArray())
			return {};
		const auto _PArray = _Ptr;
		size_t idx, max;
		yyjson_val* _Object;
		yyjson_arr_foreach(_PArray, idx, max, _Object)
			_ret.emplace_back(_Object);
		return _ret;
	}
	[[nodiscard]] size_t GetSize() const
	{
		return yyjson_get_len(_Ptr);
	}
	[[nodiscard]] size_t Size() const
	{
		return yyjson_get_len(_Ptr);
	}
	[[nodiscard]] size_t GetStringLength() const
	{
		return yyjson_get_len(_Ptr);
	}
	[[nodiscard]] MJsonValue Get(const std::string& _key) const
	{
		return yyjson_obj_get(_Ptr, _key.c_str());
	}
	[[nodiscard]] MJsonValue operator[](const std::string& _key) const
	{
		return yyjson_obj_get(_Ptr, _key.c_str());
	}
	[[nodiscard]] MJsonValue operator[](size_t _idx) const
	{
		if (!IsArray())
			return _Ptr;
		const auto _max = yyjson_arr_size(_Ptr);
		const auto _val = yyjson_arr_get_first(_Ptr);
		return _idx < _max ? _val + _idx : _val + _max - 1;
	}
	[[nodiscard]] bool Empty() const
	{
		if (!IsArray() && !IsString())
			return true;
		auto _max = yyjson_arr_size(_Ptr);
		if (IsString()) _max = yyjson_get_len(_Ptr);
		return !_max;
	}
	[[nodiscard]] size_t GetMemberCount() const
	{
		return yyjson_obj_size(_Ptr);
	}
	[[nodiscard]] std::vector<std::pair<std::string, MJsonValue>> GetMemberArray() const
	{
		std::vector<std::pair<std::string, MJsonValue>> ret;
		yyjson_val* key;
		yyjson_obj_iter iter = yyjson_obj_iter_with(_Ptr);
		while ((key = yyjson_obj_iter_next(&iter))) {
			const auto val = yyjson_obj_iter_get_val(key);
			ret.emplace_back(MJsonValue(key).GetString(), val);
		}
		return ret;
	}
	[[nodiscard]] bool HasMember(const std::string& _key) const
	{
		return yyjson_obj_get(_Ptr, _key.c_str());
	}
private:
	yyjson_val* _Ptr = nullptr;
};

class MJson
{
public:
	MJson() = default;
	MJson(const char* _path)
	{
		_document = yyjson_read_file(_path, YYJSON_READ_NOFLAG, nullptr, nullptr);
		if (!_document)
			throw std::exception("Json Parse Error !");
		root = yyjson_doc_get_root(_document);
	}
	MJson(const std::string& _data, bool _read_from_string)
	{
		if (_read_from_string)
			_document = yyjson_read(_data.c_str(), _data.length(), YYJSON_READ_NOFLAG);
		else
			_document = yyjson_read_file(_data.c_str(), YYJSON_READ_NOFLAG, nullptr, nullptr);
		if (!_document)
			throw std::exception("Json Parse Error !");
		root = yyjson_doc_get_root(_document);
	}
	~MJson()
	{
		if(_document)
		{
			yyjson_doc_free(_document);
			_document = nullptr;
			root = nullptr;
		}
	}
	MJson(MJson&& _Right) noexcept
	{
		_document = _Right._document;
		_Right._document = nullptr;
		root = yyjson_doc_get_root(_document);
	}
	MJson(const MJson& _Right) = delete;
	MJson& operator=(MJson&& _Right) noexcept
	{
		if (_document)
			yyjson_doc_free(_document);
		_document = _Right._document;
		_Right._document = nullptr;
		root = yyjson_doc_get_root(_document);
		return *this;
	}
	MJson& operator=(const MJson& _Right) = delete;
	void Parse(const std::string& _str)
	{
		_document = yyjson_read(_str.c_str(), _str.length(), YYJSON_READ_NOFLAG);
		if (!_document)
			throw std::exception("Json Parse Error !");
		root = yyjson_doc_get_root(_document);
	}
	[[nodiscard]] bool HasMember(const std::string& _key) const
	{
		return yyjson_obj_get(root, _key.c_str());
	}
	[[nodiscard]] MJsonValue Get(const std::string& _key) const
	{
		return yyjson_obj_get(root, _key.c_str());
	}
	[[nodiscard]] MJsonValue operator[](const std::string& _key) const
	{
		return yyjson_obj_get(root, _key.c_str());
	}
	[[nodiscard]] MJsonValue operator[](size_t _idx) const
	{
		if (MJsonValue(root).IsArray())
			return root;
		const auto _max = yyjson_arr_size(root);
		const auto _val = yyjson_arr_get_first(root);
		return _idx < _max ? _val + _idx : _val + _max - 1;
	}
	[[nodiscard]] bool HasParseError() const
	{
		return _document == nullptr;
	}
	[[nodiscard]] bool IsNull() const
	{
		return yyjson_is_null(root);
	}
	[[nodiscard]] bool IsBoolean() const
	{
		return yyjson_is_bool(root);
	}
	[[nodiscard]] bool IsBool() const
	{
		return yyjson_is_bool(root);
	}
	[[nodiscard]] bool IsInt() const
	{
		return yyjson_is_num(root);
	}
	[[nodiscard]] bool IsFloat() const
	{
		return yyjson_is_num(root);
	}
	[[nodiscard]] bool IsInt64() const
	{
		return yyjson_is_num(root);
	}
	[[nodiscard]] bool IsDouble() const
	{
		return yyjson_is_num(root);
	}
	[[nodiscard]] bool IsString() const
	{
		return yyjson_is_str(root);
	}
	[[nodiscard]] bool IsArray() const
	{
		return yyjson_is_arr(root);
	}
	[[nodiscard]] bool GetBool() const
	{
		return yyjson_get_bool(root);
	}
	[[nodiscard]] bool GetBoolean() const
	{
		return yyjson_get_bool(root);
	}
	[[nodiscard]] int GetInt() const
	{
		return yyjson_get_int(root);
	}
	[[nodiscard]] int64_t GetInt64() const
	{
		return yyjson_get_sint(root);
	}
	[[nodiscard]] float GetFloat() const
	{
		return float(yyjson_get_real(root));
	}
	[[nodiscard]] double GetDouble() const
	{
		return yyjson_get_real(root);
	}
	[[nodiscard]] std::string GetString() const
	{
		if (const auto _str = yyjson_get_str(root))
			return _str;
		return "";
	}
	[[nodiscard]] std::vector<MJsonValue> GetArray() const
	{
		std::vector<MJsonValue> _ret;
		if (!IsArray())
			return {};
		const auto _PArray = root;
		size_t idx, max;
		yyjson_val* _Object;
		yyjson_arr_foreach(_PArray, idx, max, _Object)
			_ret.emplace_back(_Object);
		return _ret;
	}
	[[nodiscard]] size_t GetSize() const
	{
		return yyjson_get_len(root);
	}
	[[nodiscard]] size_t Size() const
	{
		return yyjson_get_len(root);
	}
	[[nodiscard]] size_t GetStringLength() const
	{
		return yyjson_get_len(root);
	}
	[[nodiscard]] size_t GetMemberCount() const
	{
		return yyjson_obj_size(root);
	}
	[[nodiscard]] std::vector<std::pair<std::string,MJsonValue>> GetMemberArray() const
	{
		std::vector<std::pair<std::string, MJsonValue>> ret;
		yyjson_val* key;
		yyjson_obj_iter iter = yyjson_obj_iter_with(root);
		while ((key = yyjson_obj_iter_next(&iter))) {
			const auto val = yyjson_obj_iter_get_val(key);
			ret.emplace_back(MJsonValue(key).GetString(), val);
		}
		return ret;
	}
private:
	yyjson_doc* _document = nullptr;
	yyjson_val* root = nullptr;
};