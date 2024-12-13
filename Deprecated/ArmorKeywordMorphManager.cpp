#include "ArmorKeywordMorphManager.h"

std::unordered_set<RE::BGSKeyword*> DEPRECATED::ArmorKeywordMorphManager::ScanEquippedArmor(RE::Actor* a_actor, std::unordered_set<RE::TESBoundObject*> exclude_list)
{
	std::unordered_set<RE::BGSKeyword*> results;

	if (!a_actor) {
		return results;
	}

	auto process_armor = [this, &results, &exclude_list](const RE::BGSInventoryItem& a_item) -> RE::BSContainer::ForEachResult {
		if (!a_item.object || !a_item.object->Is<RE::TESObjectARMO>() || exclude_list.contains(a_item.object) /*|| !a_item.object->Is<RE::TESObjectWEAP>() */) {
			return RE::BSContainer::ForEachResult::kContinue;
		}

		auto armor = a_item.object->As<RE::TESObjectARMO>();
		for (auto& kw : armor->keywords) {
			if (kw && this->IsMorphKeyword(kw)) {
				results.insert(kw);
			}
		}

		auto instance_data = a_item.instanceData.get();
		if (!instance_data) {
			return RE::BSContainer::ForEachResult::kContinue;
		}

		auto instance_data_kw_form = instance_data->GetKeywordData();
		if (!instance_data_kw_form) {
			return RE::BSContainer::ForEachResult::kContinue;
		}

		for (auto& kw : instance_data_kw_form->keywords) {
			if (kw && this->IsMorphKeyword(kw)) {
				results.insert(kw);
			}
		}
	};

	a_actor->ForEachEquippedItem(process_armor);
}
