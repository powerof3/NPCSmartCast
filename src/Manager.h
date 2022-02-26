#pragma once

namespace Offensive
{
	class CastManager
	{
	public:
		static void Install()
		{
			Handler<RE::CombatInventoryItemMagic, 44758>::Install();
			Handler<RE::CombatInventoryItemShout, 44803>::Install();
			Handler<RE::CombatInventoryItemStaff, 44818>::Install();
			Handler<RE::CombatInventoryItemPotion, 44773>::Install();
			Handler<RE::CombatInventoryItemScroll, 44788>::Install();
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
				stl::asm_replace(func.address(), 0x2E, CanCast);
				logger::info("Installed {}"sv, typeid(Handler).name());
			}

		private:
			Handler() = delete;
			Handler(const Handler&) = delete;
			Handler(Handler&&) = delete;

			~Handler() = delete;

			Handler& operator=(const Handler&) = delete;
			Handler& operator=(Handler&&) = delete;

			static bool CanCast(RE::CombatInventoryItemMagicT<T, RE::CombatMagicCasterOffensive>* a_this, RE::CombatController* a_controller)
			{
				return !a_controller->timers->fleeing ?
                           detail::SmartCast(a_this, a_controller) :
                           false;
			}

			struct detail
			{
				static bool SmartCast(RE::CombatInventoryItemMagicT<T, RE::CombatMagicCasterOffensive>* a_this, const RE::CombatController* a_controller)
				{
					auto actor = a_controller->actor;
					if (!actor) {
						actor = a_controller->actorHandle.get();
					}

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

					auto target = a_controller->currentTarget;
					if (!target) {
						target = a_controller->currentTargetHandle.get();
					}

					const auto effect = a_this->effect;
					const auto mgef = effect ? effect->baseEffect : nullptr;
					if (target && mgef) {
						const auto resistVar = mgef->data.resistVariable;

						const auto get_ele_weakness = [&]() {
							std::vector<std::pair<float, RE::ActorValue>> vec{
								{ target->GetActorValue(RE::ActorValue::kResistFire), RE::ActorValue::kResistFire },
								{ target->GetActorValue(RE::ActorValue::kResistFrost), RE::ActorValue::kResistFrost },
								{ target->GetActorValue(RE::ActorValue::kResistFrost), RE::ActorValue::kResistShock },
								{ target->GetActorValue(RE::ActorValue::kResistFrost), RE::ActorValue::kPoisonResist }
							};

							if (std::ranges::all_of(vec, [](const auto& val) { return val.first == 0.0f; })) {
								return RE::ActorValue::kNone;
							}
							return std::ranges::min_element(vec, [](const auto& a, const auto& b) { return a.first < b.first; })->second;
						};

						auto weakness = get_ele_weakness();
						if (weakness == resistVar) {
							return true;
						}

						const auto has_ele_spell = [&](const RE::CombatInventoryController* a_invController, RE::ActorValue a_av) {
							const auto vec = a_invController->inventoryItems[0];  //seems to be the main one

							const auto type = a_this->GetType();
							return std::ranges::any_of(vec, [&](const auto& invItem) {
								if (invItem && invItem->GetType() == type) {
									const auto invItemMagic = static_cast<RE::CombatInventoryItemMagic*>(invItem.get());
									const auto spell = invItemMagic ? invItemMagic->GetMagicItem() : nullptr;
									const auto avMgef = spell ? spell->avEffectSetting : nullptr;

									return avMgef && avMgef->data.resistVariable == a_av;
								}
								return false;
							});
						};

						if (weakness != RE::ActorValue::kNone && has_ele_spell(a_controller->inventoryController, weakness)) {
							return false;
						}

						const auto get_target_weakness = [&]() {
							const auto magic = target->GetActorValue(RE::ActorValue::kMagicka);
							const auto stamina = target->GetActorValue(RE::ActorValue::kStamina);

							const auto rightHand = target->GetEquippedObject(false);
							const auto leftHand = target->GetEquippedObject(true);

							if (rightHand && rightHand->Is(RE::FormType::Spell) || leftHand && leftHand->Is(RE::FormType::Spell) || magic > stamina) {
								return RE::ActorValue::kResistShock;
							}
							return RE::ActorValue::kResistFrost;
						};

						weakness = get_target_weakness();
						if (weakness == resistVar) {
							return true;
						}
						if (has_ele_spell(a_controller->inventoryController, weakness)) {
							return false;
						}
					}

					return true;
				}
			};
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
