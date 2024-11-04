#pragma once
#include "SFEventHandler.h"
#include "ChargenUtils.h"

/*
* Thread-safe class for placing markers on committed morph offsets
*/
class ActorDynamicMorphManager
{
public:
	static constexpr std::string_view offsetPrefix{ "ECOffset_" };
	static constexpr std::string_view overweightMorphName{ "Overweight" };
	static constexpr std::string_view strongMorphName{ "Strong" };
	static constexpr std::string_view thinMorphName{ "Thin" };
	static constexpr std::string_view overweightOffsetName{ "ECOffset_Overweight" };
	static constexpr std::string_view strongOffsetName{ "ECOffset_Strong" };
	static constexpr std::string_view thinOffsetName{ "ECOffset_Thin" };

	static ActorDynamicMorphManager* GetSingleton()
	{
		static ActorDynamicMorphManager singleton;
		return &singleton;
	}

	inline std::string GetOffsetName(std::string_view morph_name)
	{
		return std::string(offsetPrefix) + morph_name.data();
	}

	void MorphOffsetCommit(RE::Actor* actor, std::string_view morph_name, float offset)
	{
		/*ActorMutexMap::accessor accessor;
		if (actor_mutexes_.insert(accessor, actor)) {
			std::shared_ptr<std::shared_mutex> wrapper = std::make_shared<std::shared_mutex>();
			accessor->second = wrapper;
		}*/

		{
			//std::scoped_lock lock(*accessor->second);
			auto npc = actor->GetNPC();
			RE::BSFixedStringCS morph_offset_name = GetOffsetName(morph_name);
			if (!npc->shapeBlendData) {
				npc->shapeBlendData = new RE::BSTHashMap<RE::BSFixedStringCS, float>();
			}

			(*npc->shapeBlendData)[morph_offset_name] += offset;
			ChangeMorphValue_Impl(actor, morph_name, offset);
		}
	}

	bool ReevaluateMorph(RE::Actor* actor, std::unordered_map<std::string, float> new_offset_batch) {
		{
			//std::scoped_lock lock(*accessor->second);
			bool morph_changed = false;
			auto npc = actor->GetNPC();
			if (!npc->shapeBlendData) {
				if (new_offset_batch.empty()) {
					return morph_changed;
				} else {
					npc->shapeBlendData = new RE::BSTHashMap<RE::BSFixedStringCS, float>();
				}
			}

			auto&                            morph_data = (*npc->shapeBlendData);
			std::vector<RE::BSFixedStringCS> morphs_to_remove;
			for (auto it = morph_data.begin(); it != morph_data.end(); ++it) {
				if (it->key.contains(offsetPrefix)) {
					float            offset = it->value;
					std::string morph_name = it->key.data() + offsetPrefix.size();

					if (!new_offset_batch.contains(morph_name)) {
						morphs_to_remove.emplace_back(it->key);
						ChangeMorphValue_Impl(actor, morph_name, -offset);
						morph_changed = true;
					} else if (float new_offset = new_offset_batch[morph_name]; new_offset != offset) {
						new_offset_batch.erase(morph_name);
						it->value = new_offset;
						ChangeMorphValue_Impl(actor, morph_name, new_offset - offset);
						morph_changed = true;
					} else {
						// If the offset exists and is the same, do nothing
					}
				}
			}
			for (auto& morph_name : morphs_to_remove) {
				morph_data.erase(morph_name);  // Doesn't seem quite efficient, erase doesn't return new iterator
			}
			for (auto& [morph_name, offset] : new_offset_batch) {
				RE::BSFixedStringCS morph_offset_name = GetOffsetName(morph_name);
				(*npc->shapeBlendData)[morph_offset_name] += offset;
				ChangeMorphValue_Impl(actor, morph_name, offset);
				morph_changed = true;
			}

			return morph_changed;
		}
	}

	bool RestoreMorph(RE::Actor* actor)
	{
		/*ActorMutexMap::accessor accessor;
		if (actor_mutexes_.insert(accessor, actor)) {
			std::shared_ptr<std::shared_mutex> wrapper = std::make_shared<std::shared_mutex>();
			accessor->second = wrapper;
		}*/

		{
			//std::scoped_lock lock(*accessor->second);
			bool morph_changed = false;
			auto npc = actor->GetNPC();
			if (!npc->shapeBlendData)
				return morph_changed;

			auto& morph_data = (*npc->shapeBlendData); 
			std::vector<RE::BSFixedStringCS> morphs_to_remove;
			for (auto it = morph_data.begin(); it != morph_data.end();)
			{
				if (it->key.contains(offsetPrefix))
				{
					float offset = it->value;
					std::string_view morph_name = it->key.data() + offsetPrefix.size();
					morphs_to_remove.emplace_back(it->key);
					ChangeMorphValue_Impl(actor, morph_name, -offset);
					morph_changed = true;
				}
				++it;
			}
			for (auto& morph_name : morphs_to_remove)
			{
				morph_data.erase(morph_name); // Doesn't seem quite efficient, erase doesn't return new iterator
			}

			return morph_changed;
		}
	}

private:
	ActorDynamicMorphManager() = default;

	/*using ActorMutexMap = tbb::concurrent_hash_map<RE::Actor*, std::shared_ptr<std::shared_mutex>>;
	ActorMutexMap actor_mutexes_;*/

	static void ChangeMorphValue_Impl(RE::Actor* actor, std::string_view morph_name, float offset)
	{
		auto npc = actor->GetNPC();

		if (morph_name == overweightMorphName)
		{
			npc->morphWeight.fat += offset;
		}
		else if (morph_name == strongMorphName)
		{
			npc->morphWeight.muscular += offset;
		}
		else if (morph_name == thinMorphName)
		{
			npc->morphWeight.thin += offset;
		}
		else
		{
			for (auto& [key, value] : *npc->shapeBlendData)
			{
				if (key == morph_name)
				{
					value += offset;
					break;
				}
			}
		}
	}
};