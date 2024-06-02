#pragma once

#ifndef LibSvcApi
#ifdef MoeVSDll
#ifdef LibSvcDll
#define LibSvcApi __declspec(dllexport)
#else
#ifndef MoeVS
#define LibSvcApi __declspec(dllimport)
#else
#define LibSvcApi
#endif
#endif
#else
#define LibSvcApi
#endif
#endif