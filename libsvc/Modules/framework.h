#pragma once

#ifdef LibSvcDll
#define LibSvcApi __declspec(dllexport)
#else
#ifndef MoeVS
#define LibSvcApi __declspec(dllimport)
#else
#define LibSvcApi
#endif
#endif