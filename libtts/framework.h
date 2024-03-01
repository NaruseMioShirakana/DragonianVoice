#pragma once
#ifdef LIBTTS_EXPORTS
#define LibTTSApi __declspec(dllexport)
#else
#define LibTTSApi __declspec(dllimport)
#endif