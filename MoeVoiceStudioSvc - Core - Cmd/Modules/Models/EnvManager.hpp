/**
 * FileName: EnvManager.hpp
 * Note: MoeVoiceStudioCore 环境管理
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
 * date: 2022-10-17 Create
*/

#pragma once
#include <onnxruntime_cxx_api.h>

#define MoeVoiceStudioCoreEnvManagerHeader namespace moevsenv{
#define MoeVoiceStudioCoreEnvManagerEnd }

MoeVoiceStudioCoreEnvManagerHeader
class MoeVoiceStudioEnv
{
public:
	MoeVoiceStudioEnv() = default;
	~MoeVoiceStudioEnv() { Destory(); }
	void Load(unsigned ThreadCount, unsigned DeviceID, unsigned Provider);
	void Destory();
	[[nodiscard]] bool IsEnabled() const;
	[[nodiscard]] Ort::Env* GetEnv() const { return GlobalOrtEnv; }
	[[nodiscard]] Ort::SessionOptions* GetSessionOptions() const { return GlobalOrtSessionOptions; }
	[[nodiscard]] Ort::MemoryInfo* GetMemoryInfo() const { return GlobalOrtMemoryInfo; }
	[[nodiscard]] int GetCurThreadCount() const { return (int)CurThreadCount; }
	[[nodiscard]] int GetCurDeviceID() const { return (int)CurDeviceID; }
	[[nodiscard]] int GetCurProvider() const { return (int)CurProvider; }
private:
	void Create(unsigned ThreadCount_, unsigned DeviceID_, unsigned ExecutionProvider_);
	Ort::Env* GlobalOrtEnv = nullptr;
	Ort::SessionOptions* GlobalOrtSessionOptions = nullptr;
	Ort::MemoryInfo* GlobalOrtMemoryInfo = nullptr;
	unsigned CurThreadCount = unsigned(-1);
	unsigned CurDeviceID = unsigned(-1);
	unsigned CurProvider = unsigned(-1);
	OrtCUDAProviderOptionsV2* cuda_option_v2 = nullptr;
};

MoeVoiceStudioEnv& GetGlobalMoeVSEnv();

MoeVoiceStudioCoreEnvManagerEnd