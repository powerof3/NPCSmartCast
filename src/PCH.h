#pragma once

#define WIN32_LEAN_AND_MEAN

#define NOMINMAX

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#pragma warning(push)
#include <spdlog/sinks/basic_file_sink.h>
#include <xbyak/xbyak.h>
#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

namespace logger = SKSE::log;
namespace numeric = SKSE::stl::numeric;
namespace string = SKSE::stl::string;

using namespace std::literals;

using RNG = SKSE::stl::RNG;

namespace stl
{
	using namespace SKSE::stl;

	void asm_replace(std::uintptr_t a_from, std::size_t a_size, std::uintptr_t a_to);

	template <class F>
	void asm_replace(std::uintptr_t a_from, std::size_t a_size, F a_newFunc)
	{
		asm_replace(a_from, a_size, reinterpret_cast<std::uintptr_t>(a_newFunc));
	}

	template <class F, std::size_t idx, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(idx, T::thunk);
	}
}

#ifdef SKYRIM_AE
#	define REL_ID(se, ae) ae
#	define OFFSET(se, ae) ae
#else
#	define REL_ID(se, ae) se
#	define OFFSET(se, ae) se
#endif

#include "Version.h"
