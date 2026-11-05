#include "Papyrus.h"

#include "Layers.h"
#include "Settings.h"

namespace Papyrus
{
	namespace
	{
		constexpr auto kClass = "LiquidLayersNG"sv;

		void Climax(RE::StaticFunctionTag*, RE::Actor* a_giver, RE::Actor* a_receiver, std::int32_t a_kind)
		{
			if (a_kind < static_cast<std::int32_t>(Kind::kVaginal) || a_kind > static_cast<std::int32_t>(Kind::kFacial)) {
				return;
			}
			Layers::Climax(a_giver, a_receiver, static_cast<Kind>(a_kind));
		}

		void Wash(RE::StaticFunctionTag*, RE::Actor* a_actor)
		{
			if (Settings::Get().washBathing) {
				Layers::WashOff(a_actor, "bathing");
			}
		}

		// Not callable from tasklets, so the VM runs it on the main thread, where Layers keeps its state.
		std::int32_t Wipe(RE::StaticFunctionTag*, RE::Actor* a_actor)
		{
			return Layers::WipeOff(a_actor);
		}

		bool Register(RE::BSScript::IVirtualMachine* a_vm)
		{
			a_vm->RegisterFunction("Climax"sv, kClass, Climax);
			a_vm->RegisterFunction("Wash"sv, kClass, Wash);
			a_vm->RegisterFunction("Wipe"sv, kClass, Wipe);
			return true;
		}
	}

	void Install()
	{
		SKSE::GetPapyrusInterface()->Register(Register);
	}
}
