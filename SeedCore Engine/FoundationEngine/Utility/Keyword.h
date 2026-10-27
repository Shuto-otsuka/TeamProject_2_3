#pragma once

#ifdef SEEDCORE_EXPORTS
#define SEEDCORE_API __declspec(dllexport)
#else
#define SEEDCORE_API __declspec(dllimport)
#endif

#define SC_IN _In_
#define SC_OUT _Out_
#define SC_INOUT _Inout_

#define SC_DEBUG _DEBUG
#define SC_RELEASE !_DEBUG

#define SC_CONSTEX static constexpr

#define NODISCARD [[nodiscard]]
#define FALLTHROUGH [[fallthrough]]

#ifndef SC_INVALID
#define SC_INVALID 0xFFFFFFFF
#endif