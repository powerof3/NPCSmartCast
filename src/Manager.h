#pragma once

#include "Util.h"

namespace Offensive
{
	class CastManager
	{
	public:
		static void Install()
		{
			Handler<RE::CombatInventoryItemMagic, REL_ID(44758, 46419)>::Install();
			Handler<RE::CombatInventoryItemShout, REL_ID(44803,46408)>::Install();
			Handler<RE::CombatInventoryItemStaff, REL_ID(44818, 46396)>::Install();
			Handler<RE::CombatInventoryItemPotion, REL_ID(44773, 46386)>::Install();
			Handler<RE::CombatInventoryItemScroll, REL_ID(44788, 46374)>::Install();

			logger::info("Installed {}"sv, typeid(CastManager).name());
		}

	protected:
		template <class T, std::uint64_t ID>
		class Handler
		{
		public:
			static void Install()
			{
				REL::Relocation<std::uintptr_t> func{ REL::ID(ID) };
				stl::asm_replace(func.address(), 0x2E, CheckShouldEquip);
				logger::info("Installed {}"sv, typeid(Handler).name());
			}

		private:
			Handler() = delete;
			Handler(const Handler&) = delete;
			Handler(Handler&&) = delete;

			~Handler() = delete;

			Handler& operator=(const Handler&) = delete;
			Handler& operator=(Handler&&) = delete;

			static bool CheckShouldEquip(RE::CombatInventoryItemMagicT<T, RE::CombatMagicCasterOffensive>* a_this, RE::CombatController* a_controller)
			{
				if (a_controller->state->isFleeing) {
					return false;
				}

				const auto actor = a_controller->handleCount ? a_controller->cachedActor : a_controller->actorHandle.get();

				if constexpr (std::is_same_v<T, RE::CombatInventoryItemShout>) {
					if (actor) {
						const auto process = actor->currentProcess;
						const auto highProcess = process ? process->high : nullptr;
						const auto voiceTimer = highProcess ? highProcess->voiceRecoveryTime : 0.0f;

						if (voiceTimer < RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatInventoryShoutMaxRecoveryTime")->GetFloat()) {
							return false;
						}
					}
				}

				const auto target = a_controller->handleCount ? a_controller->cachedTarget : a_controller->targetHandle.get();
				const auto mgef = a_this->effect ? a_this->effect->baseEffect : nullptr;

				if (target && mgef) {
					const auto resistVar = mgef->data.resistVariable;

					auto weakness = util::offensive::get_elemental_weakness(target);
					if (weakness == resistVar) {
						return true;
					}
					if (weakness != RE::ActorValue::kNone && util::offensive::has_elemental_spell(a_this->GetType(), a_controller->inventory, weakness)) {
						return false;
					}

					weakness = util::offensive::get_target_weakness(target);
					if (weakness == resistVar) {
						return true;
					}
					if (util::offensive::has_elemental_spell(a_this->GetType(), a_controller->inventory, weakness)) {
						return false;
					}
				}

				return true;
			}
		};

	private:
		constexpr CastManager() noexcept = default;
		CastManager(const CastManager&) = delete;
		CastManager(CastManager&&) = delete;

		~CastManager() = default;

		CastManager& operator=(const CastManager&) = delete;
		CastManager& operator=(CastManager&&) = delete;
	};
}
