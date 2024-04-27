#include "helper.h"
#include "Console.hpp"
#include "Game.hpp"
#include <memory>

namespace DX11_Base {

	class Hooking2
	{
	public:
		void						Hook2();
		void						Unhook2();

	public:
		explicit Hooking2();
		~Hooking2() noexcept;
		Hooking2(Hooking2 const&) = delete;
		Hooking2(Hooking2&&) = delete;
		Hooking2& operator=(Hooking2 const&) = delete;
		Hooking2& operator=(Hooking2&&) = delete;

	};

	inline std::unique_ptr<Hooking2> g_Hooking;
	
}