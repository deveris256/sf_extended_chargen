#include "ArmorKeywordMorphManager.h"

std::vector<RE::BGSKeyword*> ExtendedChargen::ArmorKeywordMorphManager::ScanEquippedArmor(RE::Actor* a_actor)
{
	std::vector<RE::BGSKeyword*> results;

	if (!a_actor) {
		return results;
	}

	auto process_armor = [this, &results](const RE::BGSInventoryItem& a_item) -> RE::BSContainer::ForEachResult {
		if (!a_item.object || !a_item.object->Is<RE::TESObjectARMO>() /*|| !a_item.object->Is<RE::TESObjectWEAP>() */) {
			return RE::BSContainer::ForEachResult::kContinue;
		}

		auto armor = a_item.object->As<RE::TESObjectARMO>();
		for (auto& kw : armor->keywords) {
			if (kw && this->IsMorphKeyword(kw)) {
				results.push_back(kw);
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
				results.push_back(kw);
			}
		}
	};

	a_actor->ForEachEquippedItem(process_armor);
}
