#pragma once

namespace util
{
	inline std::pair<RE::ActorValue, float> get_av(const RE::NiPointer<RE::Actor>& a_target, RE::ActorValue a_av)
	{
		return { a_av, a_target->GetActorValue(a_av) };
	}

	namespace offensive
	{
		inline RE::ActorValue get_elemental_weakness(const RE::NiPointer<RE::Actor>& a_target)
		{
			std::map<RE::ActorValue, float> av_map{
				{ get_av(a_target, RE::ActorValue::kResistFire) },
				{ get_av(a_target, RE::ActorValue::kResistFrost) },
				{ get_av(a_target, RE::ActorValue::kResistShock) },
				{ get_av(a_target, RE::ActorValue::kPoisonResist) }
			};

			if (std::ranges::all_of(av_map, [](const auto& av) { return av.second == 0.0f; })) {
				return RE::ActorValue::kNone;
			}

			return std::ranges::min_element(av_map, [](const auto& a_av, const auto& b_av) { return a_av.second < b_av.second; })->first;
		}

		inline bool has_elemental_spell(const RE::CombatInventoryItem::TYPE a_type, const RE::CombatInventory* a_combatInventory, RE::ActorValue a_av)
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

		inline RE::ActorValue get_target_weakness(const RE::NiPointer<RE::Actor>& a_target)
		{
			const auto magic = a_target->GetActorValue(RE::ActorValue::kMagicka);
			const auto stamina = a_target->GetActorValue(RE::ActorValue::kStamina);

			const auto rightHand = a_target->GetEquippedObject(false);
			const auto leftHand = a_target->GetEquippedObject(true);

			return (rightHand && rightHand->Is(RE::FormType::Spell) || leftHand && leftHand->Is(RE::FormType::Spell) || magic > stamina) ?
			           RE::ActorValue::kResistShock :  //target is mage
                       RE::ActorValue::kResistFrost;   //target is warrior
		}
	}
}
