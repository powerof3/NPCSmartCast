#pragma once

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

			struct detail
			{
				static RE::ActorValue get_elemental_weakness(RE::Actor* a_target)
				{
					std::map<RE::ActorValue, float> av_map{
						{ RE::ActorValue::kResistFire, a_target->GetActorValue(RE::ActorValue::kResistFire) },
						{ RE::ActorValue::kResistFrost, a_target->GetActorValue(RE::ActorValue::kResistFrost) },
						{ RE::ActorValue::kResistShock, a_target->GetActorValue(RE::ActorValue::kResistShock) },
						{ RE::ActorValue::kPoisonResist, a_target->GetActorValue(RE::ActorValue::kPoisonResist) }
					};

					if (std::ranges::all_of(av_map, [](const auto& av) { return av.second == 0.0f; })) {
						return RE::ActorValue::kNone;
					}

					return std::ranges::min_element(av_map, [](const auto& a_av, const auto& b_av) { return a_av.second < b_av.second; })->first;
				}

				static bool has_elemental_spell(const RE::CombatInventoryItem::TYPE a_type, const RE::CombatInventory* a_combatInventory, RE::ActorValue a_av)
				{
					const auto& vec = a_combatInventory->inventoryItems[0];  //first category

					return std::ranges::any_of(vec, [&](const auto& invItem) {
						if (invItem && invItem->GetType() == a_type) {
							const auto invItemMagic = static_cast<RE::CombatInventoryItemMagic*>(invItem.get());
							const auto spell = invItemMagic ? invItemMagic->GetMagic() : nullptr;
							const auto avMgef = spell ? spell->avEffectSetting : nullptr;

							return avMgef && avMgef->data.resistVariable == a_av;
						}
						return false;
					});
				}

				static RE::ActorValue get_target_weakness(RE::Actor* a_target)
				{
					const auto magic = a_target->GetActorValue(RE::ActorValue::kMagicka);
					const auto stamina = a_target->GetActorValue(RE::ActorValue::kStamina);

					const auto rightHand = a_target->GetEquippedObject(false);
					const auto leftHand = a_target->GetEquippedObject(true);

					return (rightHand && rightHand->Is(RE::FormType::Spell) || leftHand && leftHand->Is(RE::FormType::Spell) || magic > stamina) ?
					           RE::ActorValue::kResistShock :  //target is mage
                               RE::ActorValue::kResistFrost;   //target is warrior
				}
			};

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

					auto weakness = detail::get_elemental_weakness(target.get());
					if (weakness == resistVar) {
						return true;
					}
					if (weakness != RE::ActorValue::kNone && detail::has_elemental_spell(a_this->GetType(), a_controller->inventory, weakness)) {
						return false;
					}

					weakness = detail::get_target_weakness(target.get());
					if (weakness == resistVar) {
						return true;
					}
					if (detail::has_elemental_spell(a_this->GetType(), a_controller->inventory, weakness)) {
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
