#pragma once

#ifdef LibSvcDll
#define LibSvcApi __declspec(dllexport)
#else
#define LibSvcApi __declspec(dllimport)
#endif